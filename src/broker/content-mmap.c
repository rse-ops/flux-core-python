/************************************************************\
 * Copyright 2022 Lawrence Livermore National Security, LLC
 * (c.f. AUTHORS, NOTICE.LLNS, COPYING)
 *
 * This file is part of the Flux resource manager framework.
 * For details, see https://github.com/flux-framework.
 *
 * SPDX-License-Identifier: LGPL-3.0
\************************************************************/

/* content-mmap.c - map files into content cache on rank 0
 *
 * Each file is represented by a 'struct content_region' that includes
 * a 'fileref' object containing the file's metadata and blobrefs for content.
 * The region also contains a pointer to mmap(2)ed memory for the file's
 * content.
 *
 * All files have one or more tags, so the regions are placed in a
 * hash-of-lists where the list names are tags, and the entries are struct
 * content_regions.  When files are listed or removed, the requestor provides
 * one or more tags.  (flux filemap has an implicit "main" tag, but that
 * is entirely implemented in the tools, not here).
 *
 * The general idea is that one makes a list query with tags and an optional
 * glob and gets back a list of blobrefs.  Each blobref represents a serialized
 * 'fileref' object that corresponds to a file/content_region.  Within the
 * fileref is optionally a vector of blobrefs representing data content.
 * Having obtained this list, the client can use it to pull the fileref, and
 * subsequently its data through the content cache.
 *
 * The content-cache calls content_mmap_region_lookup() on rank 0 when it
 * doesn't have a requested blobref in cache, and only consults the backing
 * store when that fails.  If content_mmap_region_lookup() succeeds, the
 * content-cache takes a reference on the struct content_region.  When we
 * request to unmap a region, the munmap(2) and free of the struct is delayed
 * until all content-cache references are dropped.
 *
 * Slightly tricky optimization:
 * To speed up content_mmap_region_lookup() we have mm->cache, which is used
 * to find a content_region given a hash.  The cache contains hash keys for
 * both mmapped data (fileref.blobvec) and serialized fileref objects.
 * A given hash may appear in multiple files or parts of the same file,
 * so when a file is mapped, we put all its hashes in mm->cache except those
 * that are already mapped.  If nothing is unmapped, then we know all the
 * blobrefs for all the files will remain valid.  However when something is
 * unmapped we could be losing pieces of unrelated files.  Since unmaps are
 * bulk operations involving tags, we just walk the entire hash-of-lists
 * at that time and restore any missing cache entries.
 *
 * Safety issue:
 * The content addressable storage model relies on the fact that once hashed,
 * data does not change.  However, this cannot be assured when the data is
 * mmapped from a file that may not be protected from updates.  To avoid
 * propagating bad data in the cache, content_mmap_validate() is called each
 * time an mmapped cache entry is accessed.  This function recomputes the
 * hash to make sure the content has not changed.  If the data has changed,
 * the content-cache returns an error to the requestor.  In addition, mmapped
 * pages could become invalid if the size of a mapped file is reduced.
 * Accessing invalid pages could cause the broker to crash with SIGBUS.  To
 * mitigate this, content_mmap_validate() also calls stat(2) on the file to
 * make sure the memory region is still valid.  This is not foolproof because
 * it is inherently a time-of-check-time-of-use problem.  In fact we rate
 * limit the calls to stat(2) to avoid a "stat storm" when a file with many
 * blobrefs is accessed, which increases the window where it could have
 * changed.  But it's likely better than not checking at all.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <fnmatch.h>
#include <jansson.h>
#include <assert.h>
#include <flux/core.h>

#include "src/common/libczmqcontainers/czmq_containers.h"
#include "src/common/libutil/hola.h"
#include "src/common/libutil/errno_safe.h"
#include "src/common/libutil/errprintf.h"
#include "src/common/libutil/blobref.h"
#include "src/common/libutil/fileref.h"
#include "src/common/libutil/monotime.h"
#include "ccan/ptrint/ptrint.h"
#include "ccan/str/str.h"

#include "content-mmap.h"

struct cache_entry {
    struct content_region *reg;
    const void *data;                       // pointer into mmapped reg->data
    size_t size;                            //   or reg->fileref_data
    void *hash;                             // contiguous with struct
};

struct content_region {
    void *data;                             // memory-mapped data
    size_t data_size;                       // memory-mapped size
    int refcount;
    json_t *fileref;
    void *fileref_data;                     // encoded fileref data
    size_t fileref_size;                    // encoded fileref size
    char *blobref;                          // blobref for fileref data

    struct content_mmap *mm;

    char *fullpath;                         // full path for stat(2) checking
    struct timespec last_check;             // rate limit stat(2) checking
};

struct content_mmap {
    flux_t *h;
    uint32_t rank;
    char *hash_name;
    flux_msg_handler_t **handlers;
    struct hola *tags;                      // tagged bundles of files/regions
    zhashx_t *cache;                        // hash digest => cache entry
};

static int content_hash_size;

static const int max_blobrefs_per_response = 1000;
static const int max_filerefs_per_response = 100;

static const double max_check_age = 5; // seconds since last region stat(2)

static void cache_entry_destroy (struct cache_entry *e)
{
    if (e) {
        int saved_errno = errno;
        free (e);
        errno = saved_errno;
    }
}

static struct cache_entry *cache_entry_create (const void *digest)
{
    struct cache_entry *e;

    if (!(e = calloc (1, sizeof (*e) + content_hash_size)))
        return NULL;
    e->hash = (char *)(e + 1);
    memcpy (e->hash, digest, content_hash_size);
    return e;
}

/* Add entry to cache.
 * If entry exists, return success.  The blobref must be valid in the
 * cache - where it comes from is unimportant.
 * N.B. this is a potential hot spot so defer memory allocation until
 * after lookup.
 */
static int cache_entry_add (struct content_region *reg,
                            const void *data,
                            size_t size,
                            const char *blobref)
{
    struct cache_entry *e;
    char digest[BLOBREF_MAX_DIGEST_SIZE];

    if (blobref_strtohash (blobref, digest, sizeof (digest)) < 0)
        return -1;
    if (zhashx_lookup (reg->mm->cache, digest) < 0)
        return 0;
    if (!(e = cache_entry_create (digest)))
        return -1;
    e->reg = reg;
    e->data = data;
    e->size = size;
    if (zhashx_insert (reg->mm->cache, e->hash, e) < 0) {
        cache_entry_destroy (e);
        return 0;
    }
    return 0;
}

/* Remove entry from cache IFF it belongs to this region.
 */
static int cache_entry_remove (struct content_mmap *mm,
                               struct content_region *reg,
                               const char *blobref)
{
    struct cache_entry *e;
    char digest[BLOBREF_MAX_DIGEST_SIZE];

    if (blobref_strtohash (blobref, digest, sizeof (digest)) < 0)
        return -1;
    if ((e = zhashx_lookup (mm->cache, digest)) // calls destructor
        && reg == e->reg)
        zhashx_delete (mm->cache, digest);
    return 0;
}

/* Remove all cache entries associated with region (blobvec + fileref).
 */
static void region_cache_remove (struct content_region *reg)
{
    if (reg->fileref) {
        int saved_errno = errno;
        json_t *blobvec = json_object_get (reg->fileref, "blobvec");

        if (blobvec) {
            size_t index;
            json_t *entry;

            json_array_foreach (blobvec, index, entry) {
                json_int_t offset;
                json_int_t size;
                const char *blobref;

                if (json_unpack (entry,
                                 "[I,I,s]",
                                 &offset,
                                 &size,
                                 &blobref) == 0)
                    cache_entry_remove (reg->mm, reg, blobref);
            }
        }
        if (reg->blobref)
            cache_entry_remove (reg->mm, reg, reg->blobref);

        errno = saved_errno;
    }
}

/* Add cache entries for entries associated with region (blobvec + fileref).
 */
static int region_cache_add (struct content_region *reg)
{
    size_t index;
    json_t *entry;
    json_t *blobvec;

    if (cache_entry_add (reg,
                         reg->fileref_data,
                         reg->fileref_size,
                         reg->blobref) < 0)
        return -1;
    if (!(blobvec = json_object_get (reg->fileref, "blobvec")))
        goto inval;
    json_array_foreach (blobvec, index, entry) {
        json_int_t offset;
        json_int_t size;
        const char *blobref;
        if (json_unpack (entry, "[I,I,s]", &offset, &size, &blobref) < 0)
            goto inval;
        if (cache_entry_add (reg,
                             reg->data + offset,
                             size,
                             blobref) < 0)
            return -1;
    }
    return 0;
inval:
    errno = EINVAL;
    return -1;
}

/* After a region is unmapped, other regions may have blobrefs that are no
 * longer represented in the cache.  This scans all mapped regions and fills
 * in missing cache entries.  Design tradeoff:  mapping and lookup are fast,
 * and the cache implementation is lightweight and simple, at the expense of
 * unmap efficiency.
 */
static int plug_cache_holes (struct content_mmap *mm)
{
    const char *name;
    struct content_region *reg;

    name = hola_hash_first (mm->tags);
    while (name) {
        reg = hola_list_first (mm->tags, name);
        while (reg) {
            if (region_cache_add (reg) < 0)
                return -1;
            reg = hola_list_next (mm->tags, name);
        }
        name = hola_hash_next (mm->tags);
    }
    return 0;
}

// zhashx_destructor_fn footprint
static void cache_entry_destructor (void **item)
{
    if (item) {
        cache_entry_destroy (*item);
        *item = NULL;
    }
}
// zhashx_hash_fn footprint
static size_t cache_entry_hasher (const void *key)
{
    return *(size_t *)key;
}
// zhashx_comparator_fn footprint
static int cache_entry_comparator (const void *item1, const void *item2)
{
    return memcmp (item1, item2, content_hash_size);
}

void content_mmap_region_decref (struct content_region *reg)
{
    if (reg && --reg->refcount == 0) {
        int saved_errno = errno;
        region_cache_remove (reg);
        if (reg->data != MAP_FAILED)
            (void)munmap (reg->data, reg->data_size);
        json_decref (reg->fileref);
        free (reg->fullpath);
        free (reg->blobref);
        free (reg->fileref_data);
        free (reg);
        errno = saved_errno;
    }
}

// zhashx_destructor_fn footprint
static void content_mmap_region_destructor (void **item)
{
    if (item) {
        content_mmap_region_decref (*item);
        *item = NULL;
    }
}

struct content_region *content_mmap_region_incref (struct content_region *reg)
{
    if (reg)
        reg->refcount++;
    return reg;
}

/* Validate mmapped blob before use, checking for:
 * - size has changed so mmmapped pages are no longer valid (SIGBUS if used!)
 * - content no longer matches hash
 * To avoid repeatedly calling stat(2) on a file, skip it if last check was
 * within max_check_age seconds.
 */
bool content_mmap_validate (struct content_region *reg,
                            const void *hash,
                            int hash_size,
                            const void *data,
                            int data_size)
{
    char hash2[BLOBREF_MAX_DIGEST_SIZE];

    // no need to check the fileref blob as it was not mmapped
    if (data == reg->fileref_data)
        return true;

    assert (reg->data != NULL);
    assert (data >= reg->data);
    assert (data + data_size <= reg->data + reg->data_size);

    if (monotime_since (reg->last_check)/1000 >= max_check_age) {
        struct stat sb;

        if (stat (reg->fullpath, &sb) < 0
            || sb.st_size < reg->data_size)
            return false;

        monotime (&reg->last_check);
    }

    if (blobref_hash_raw (reg->mm->hash_name,
                          data,
                          data_size,
                          hash2,
                          sizeof (hash2)) != hash_size
            || memcmp (hash, hash2, hash_size) != 0)
            return false;
    return true;
}

struct content_region *content_mmap_region_lookup (struct content_mmap *mm,
                                                   const void *hash,
                                                   int hash_size,
                                                   const void **data,
                                                   int *data_size)
{
    struct cache_entry *e;

    if (hash_size != content_hash_size
        || !(e = zhashx_lookup (mm->cache, hash)))
        return NULL;
    *data = e->data;
    *data_size = e->size;
    return e->reg;
}

static int fileref_encode (json_t *fileref,
                           const char *hash_name,
                           void **fileref_data,
                           size_t *fileref_size,
                           char **blobrefp)
{
    void *data;
    size_t size;
    char blobref[BLOBREF_MAX_STRING_SIZE];
    char *blobref_cpy;

    if (!(data = json_dumps (fileref, JSON_COMPACT))) {
        errno = ENOMEM;
        return -1;
    }
    size = strlen (data) + 1;
    if (blobref_hash (hash_name, data, size, blobref, sizeof (blobref)) < 0
        || !(blobref_cpy = strdup (blobref))) {
        ERRNO_SAFE_WRAP (free, data);
        return -1;
    }
    *fileref_data = data;
    *fileref_size = size;
    *blobrefp = blobref_cpy;
    return 0;
}

static struct content_region *content_mmap_region_create (
                                                     struct content_mmap *mm,
                                                     const char *path,
                                                     const char *fpath,
                                                     int chunksize,
                                                     flux_error_t *error)
{
    struct content_region *reg;

    if (!(reg = calloc (1, sizeof (*reg)))) {
        errprintf (error, "out of memory");
        goto error;
    }
    reg->refcount = 1;
    reg->mm = mm;
    reg->data = MAP_FAILED;
    if (!(reg->fullpath = strdup (fpath)))
        goto error;

    if (!(reg->fileref = fileref_create_ex (path,
                                            fpath,
                                            mm->hash_name,
                                            chunksize,
                                            &reg->data,
                                            &reg->data_size,
                                            error)))
        goto error;
    if (fileref_encode (reg->fileref,
                        mm->hash_name,
                        &reg->fileref_data,
                        &reg->fileref_size,
                        &reg->blobref) < 0) {
        errprintf (error,
                   "%s: error encoding fileref: %s",
                   path,
                   strerror (errno));
        goto error;
    }

    if (region_cache_add (reg) < 0) {
        errprintf (error,
                   "%s: error caching region blobrefs: %s",
                   path,
                   strerror (errno));
        goto error;
    }
    return reg;
error:
    content_mmap_region_decref (reg);
    return NULL;
}

static int check_string_array (json_t *o, int min_count)
{
    size_t index;
    json_t *entry;

    if (!json_is_array (o)
        || json_array_size (o) < min_count)
        goto error;
    json_array_foreach (o, index, entry) {
        if (!json_is_string (entry))
            goto error;
    }
    return 0;
error:
    errno = EPROTO;
    return -1;
}

static bool fnmatch_fileref (const char *pattern, json_t *fileref)
{
    if (pattern) {
        const char *path;

        if (json_unpack (fileref, "{s:s}", "path", &path) < 0
            || fnmatch (pattern, path, 0) != 0)
            return false;
    }
    return true;
}

static void content_mmap_add_cb (flux_t *h,
                                 flux_msg_handler_t *mh,
                                 const flux_msg_t *msg,
                                 void *arg)
{
    struct content_mmap *mm = arg;
    const char *path;
    const char *fpath = NULL;
    int chunksize;
    json_t *tags;
    struct content_region *reg = NULL;
    flux_error_t error;
    const char *errmsg = NULL;
    size_t index;
    json_t *entry;

    if (flux_request_unpack (msg,
                             NULL,
                             "{s:s s?s s:i s:o}",
                             "path", &path,
                             "fullpath", &fpath,
                             "chunksize", &chunksize,
                             "tags", &tags) < 0
        || check_string_array (tags, 1) < 0)
        goto error;
    if (mm->rank != 0) {
        errmsg = "content may only be mmapped on rank 0";
        goto inval;
    }
    if (!(reg = content_mmap_region_create (mm,
                                            path,
                                            fpath,
                                            chunksize,
                                            &error))) {
        errmsg = error.text;
        goto error;
    }
    json_array_foreach (tags, index, entry) {
        // takes a reference on region
        if (!hola_list_add_end (mm->tags, json_string_value (entry), reg))
            goto error;
    }
    if (flux_respond (h, msg, NULL) < 0)
        flux_log_error (h, "error responding to content.mmap-add request");
    content_mmap_region_decref (reg);
    return;
inval:
    errno = EINVAL;
error:
    if (flux_respond_error (h, msg, errno, errmsg) < 0)
        flux_log_error (h, "error responding to content.mmap-add request");
    content_mmap_region_decref (reg);
}

static void content_mmap_remove_cb (flux_t *h,
                                    flux_msg_handler_t *mh,
                                    const flux_msg_t *msg,
                                    void *arg)
{
    struct content_mmap *mm = arg;
    const char *errmsg = NULL;
    json_t *tags;
    size_t index;
    json_t *entry;
    int unmap_count = 0;

    if (flux_request_unpack (msg, NULL, "{s:o}", "tags", &tags) < 0
        || check_string_array (tags, 1) < 0)
        goto error;
    if (mm->rank != 0) {
        errmsg = "content can only be mmapped on rank 0";
        goto inval;
    }
    json_array_foreach (tags, index, entry) {
        if (hola_hash_delete (mm->tags, json_string_value (entry)) == 0)
            unmap_count++;
    }
    if (unmap_count > 0) {
        if (plug_cache_holes (mm) < 0) {
            errmsg = "error filling missing cache entries after unmap";
            goto error;
        }
    }
    if (flux_respond (h, msg, NULL) < 0)
        flux_log_error (h, "error responding to content.mmap-remove request");
    return;
inval:
    errno = EINVAL;
error:
    if (flux_respond_error (h, msg, errno, errmsg) < 0)
        flux_log_error (h, "error responding to content.mmap-remove request");
}

/* Get a list of files that match tag(s) and glob pattern, if any.
 * Avoid listing duplicates (e.g. a file with multiple tags) by tracking
 * fileref blobrefs in a JSON object.  Send data in zero or more responses
 * (respecting max_blobrefs_per_response or max_filerefs_per_response)
 * terminated by ENODATA.
 */
static void content_mmap_list_cb (flux_t *h,
                                  flux_msg_handler_t *mh,
                                  const flux_msg_t *msg,
                                  void *arg)
{
    struct content_mmap *mm = arg;
    const char *errmsg = NULL;
    int blobref;
    json_t *tags;
    const char *pattern = NULL;
    json_t *files = NULL;
    json_t *uniqhash = NULL;
    struct content_region *reg;
    size_t index;
    json_t *entry;

    if (flux_request_unpack (msg,
                             NULL,
                             "{s:b s:o s?s}",
                             "blobref", &blobref,
                             "tags", &tags,
                             "pattern", &pattern) < 0
        || check_string_array (tags, 1) < 0)
        goto error;
    if (!flux_msg_is_streaming (msg)) {
        errno = EPROTO;
        goto error;
    }
    if (mm->rank != 0) {
        errmsg = "content can only be mmapped on rank 0";
        errno = EINVAL;
        goto error;
    }
    if (!(files = json_array ())
        || !(uniqhash = json_object ())) {
        json_decref (files);
        goto nomem;
    }
    json_array_foreach (tags, index, entry) {
        const char *name = json_string_value (entry);

        reg = hola_list_first (mm->tags, name);
        while (reg) {
            if (!json_object_get (uniqhash, reg->blobref)
                 && fnmatch_fileref (pattern, reg->fileref)) {
                json_t *o;
                if (json_object_set (uniqhash, reg->blobref, json_null ()) < 0)
                    goto nomem;
                if (blobref) {
                    if (!(o = json_string (reg->blobref))
                        || json_array_append_new (files, o) < 0) {
                        json_decref (o);
                        goto nomem;
                    }
                }
                else {
                    if (json_array_append (files, reg->fileref) < 0)
                        goto nomem;
                }
                if (json_array_size (files) == (blobref ?
                                                max_blobrefs_per_response :
                                                max_filerefs_per_response)) {
                    if (flux_respond_pack (h, msg, "{s:O}", "files", files) < 0)
                        goto error;
                    if (json_array_clear (files) < 0)
                        goto nomem;
                }
            }
            reg = hola_list_next (mm->tags, name);
        }
    }
    if (json_array_size (files) > 0) {
        if (flux_respond_pack (h, msg, "{s:O}", "files", files) < 0)
            goto error;
    }
    if (flux_respond_error (h, msg, ENODATA, NULL) < 0) // end of stream
        flux_log_error (h, "error responding to content.mmap-list request");
    json_decref (files);
    json_decref (uniqhash);
    return;
nomem:
    errno = ENOMEM;
error:
    if (flux_respond_error (h, msg, errno, errmsg) < 0)
        flux_log_error (h, "error responding to content.mmap-list request");
    json_decref (files);
    json_decref (uniqhash);
}

static const struct flux_msg_handler_spec htab[] = {
    {
        FLUX_MSGTYPE_REQUEST,
        "content.mmap-add",
        content_mmap_add_cb,
        0
    },
    {
        FLUX_MSGTYPE_REQUEST,
        "content.mmap-remove",
        content_mmap_remove_cb,
        0
    },
    {
        FLUX_MSGTYPE_REQUEST,
        "content.mmap-list",
        content_mmap_list_cb,
        0
    },
    FLUX_MSGHANDLER_TABLE_END,
};

void content_mmap_destroy (struct content_mmap *mm)
{
    if (mm) {
        int saved_errno = errno;
        flux_msg_handler_delvec (mm->handlers);
        hola_destroy (mm->tags);
        zhashx_destroy (&mm->cache);
        free (mm->hash_name);
        free (mm);
        errno = saved_errno;
    }
}

struct content_mmap *content_mmap_create (flux_t *h,
                                          const char *hash_name,
                                          int hash_size)
{
    struct content_mmap *mm;

    if (!(mm = calloc (1, sizeof (*mm))))
        return NULL;
    content_hash_size = hash_size;
    if (flux_get_rank (h, &mm->rank) < 0)
        goto error;
    if (!(mm->hash_name = strdup (hash_name)))
        goto error;
    mm->h = h;
    if (flux_msg_handler_addvec (h, htab, mm, &mm->handlers) < 0)
        goto error;
    if (!(mm->tags = hola_create (HOLA_AUTOCREATE)))
        goto error;
    hola_set_list_destructor (mm->tags, content_mmap_region_destructor);
    hola_set_list_duplicator (mm->tags,
                         (zlistx_duplicator_fn *)content_mmap_region_incref);
    if (!(mm->cache = zhashx_new ())) {
        errno = ENOMEM;
        goto error;
    }
    zhashx_set_destructor (mm->cache, cache_entry_destructor);
    zhashx_set_key_hasher (mm->cache, cache_entry_hasher);
    zhashx_set_key_comparator (mm->cache, cache_entry_comparator);
    zhashx_set_key_destructor (mm->cache, NULL); // key is part of entry
    zhashx_set_key_duplicator (mm->cache, NULL); // key is part of entry
    return mm;
error:
    content_mmap_destroy (mm);
    return NULL;
}

// vi:ts=4 sw=4 expandtab
