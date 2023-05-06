==================
flux_kvs_lookup(3)
==================


SYNOPSIS
========

::

   #include <flux/core.h>

::

   flux_future_t *flux_kvs_lookup (flux_t *h, const char *ns, int flags,
                                   const char *key);

::

   flux_future_t *flux_kvs_lookupat (flux_t *h, int flags,
                                     const char *key, const char *treeobj);

::

   int flux_kvs_lookup_get (flux_future_t *f, const char **value);

::

   int flux_kvs_lookup_get_unpack (flux_future_t *f, const char *fmt, ...);

::

   int flux_kvs_lookup_get_raw (flux_future_t *f,
                                const void **data, int *len);

::

   int flux_kvs_lookup_get_dir (flux_future_t *f,
                                const flux_kvsdir_t **dir);

::

   int flux_kvs_lookup_get_treeobj (flux_future_t *f, const char **treeobj);

::

   int flux_kvs_lookup_get_symlink (flux_future_t *f, const char **ns,
                                    const char **target);

::

   const char *flux_kvs_lookup_get_key (flux_future_t *f);

::

   int flux_kvs_lookup_cancel (flux_future_t *f);


DESCRIPTION
===========

The Flux Key Value Store is a general purpose distributed storage
service used by Flux services.

``flux_kvs_lookup()`` sends a request to the KVS service to look up
*key* in namespace *ns*. It returns a ``flux_future_t`` object which
acts as handle for synchronization and container for the result. The
namespace *ns* is optional. If set to NULL, ``flux_kvs_lookup()`` uses
the default namespace, or if set, the namespace from the
FLUX_KVS_NAMESPACE environment variable. *flags* modifies the request
as described below.

``flux_kvs_lookupat()`` is identical to ``flux_kvs_lookup()`` except
*treeobj* is a serialized RFC 11 object that references a particular
static set of content within the KVS, effectively a snapshot.
See ``flux_kvs_lookup_get_treeobj()`` below.

All the functions below are variations on a common theme. First they
complete the lookup RPC by blocking on the response, if not already received.
Then they interpret the result in different ways. They may be called more
than once on the same future, and they may be intermixed to probe a result
or interpret it in different ways. Results remain valid until
``flux_future_destroy()`` is called.

``flux_kvs_lookup_get()`` interprets the result as a value. If the value
has length greater than zero, a NULL is appended and it is assigned
to *value*, otherwise NULL is assigned to *value*.

``flux_kvs_lookup_get_unpack()`` interprets the result as a value, which
it decodes as JSON according to variable arguments in Jansson
``json_unpack()`` format.

``flux_kvs_lookup_get_raw()`` interprets the result as a value. If the value
has length greater than zero, the value and its length are assigned to
*buf* and *len*, respectively. Otherwise NULL and zero are assigned.

``flux_kvs_lookup_get_dir()`` interprets the result as a directory,
e.g. in response to a lookup with the FLUX_KVS_READDIR flag set.
The directory object is assigned to *dir*.

``flux_kvs_lookup_get_treeobj()`` interprets the result as any RFC 11 object.
The object in JSON-encoded form is assigned to *treeobj*. Since all
lookup requests return an RFC 11 object of one type or another, this
function should work on all.

``flux_kvs_lookup_get_symlink()`` interprets the result as a symlink target,
e.g. in response to a lookup with the FLUX_KVS_READLINK flag set.
The result is parsed and symlink namespace is assigned to *ns* and
the symlink target is assigned to *target*. If a namespace was not assigned
to the symlink, *ns* is set to NULL.

``flux_kvs_lookup_get_key()`` accesses the key argument from the original
lookup.

``flux_kvs_lookup_cancel()`` cancels a stream of lookup responses
requested with FLUX_KVS_WATCH or a waiting lookup response with
FLUX_KVS_WAITCREATE. See FLAGS below for additional information.

These functions may be used asynchronously. See :man3:`flux_future_then` for
details.


FLAGS
=====

The following are valid bits in a *flags* mask passed as an argument
to ``flux_kvs_lookup()`` or ``flux_kvs_lookupat()``.

FLUX_KVS_READDIR
   Look up a directory, not a value. The lookup fails if the key does
   not refer to a directory object.

FLUX_KVS_READLINK
   If key is a symlink, read the link value. The lookup fails if the key
   does not refer to a symlink object.

FLUX_KVS_TREEOBJ
   All KVS lookups return an RFC 11 tree object. This flag requests that
   they be returned without conversion. That is, a "valref" will not
   be converted to a "val" object, and a "dirref" will not be converted
   to a "dir" object. This is useful for obtaining a snapshot reference
   that can be passed to ``flux_kvs_lookupat()``.

FLUX_KVS_WATCH
   After the initial response, continue to send responses to the lookup
   request each time *key* is mentioned verbatim in a committed transaction.
   After receiving a response, ``flux_future_reset()`` should be used to
   consume a response and prepare for the next one. Responses continue
   until the namespace is removed, the key is removed, the lookup is
   canceled with ``flux_kvs_lookup_cancel()``, or an error occurs. After
   calling ``flux_kvs_lookup_cancel()``, callers should wait for the future
   to be fulfilled with an ENODATA error to ensure the cancel request has
   been received and processed.

FLUX_KVS_WATCH_UNIQ
   Specified along with FLUX_KVS_WATCH, this flag will alter watch
   behavior to only respond when *key* is mentioned verbatim in a
   committed transaction and the value of the key has changed.

FLUX_KVS_WATCH_APPEND
   Specified along with FLUX_KVS_WATCH, this flag will alter watch
   behavior to only respond when *key* is mentioned verbatim in a
   committed transaction and the key has been appended to. The response
   will only contain the additional appended data. Note that only data
   length is considered for appends and no guarantee is made that prior
   data hasn't been overwritten.

FLUX_KVS_WATCH_FULL
   Specified along with FLUX_KVS_WATCH, this flag will alter watch
   behavior to respond when the value of the key being watched has
   changed. Unlike FLUX_KVS_WATCH_UNIQ, the key being watched need not
   be mentioned in a transaction. This may occur under several
   scenarios, such as a parent directory being altered.

FLUX_KVS_WAITCREATE
   If a KVS key does not exist, wait for it to exist before returning.
   This flag can be specified with or without FLUX_KVS_WATCH. The lookup
   can be canceled with ``flux_kvs_lookup_cancel()``. After calling
   ``flux_kvs_lookup_cancel()``, callers should wait for the future to be
   fulfilled with an ENODATA error to ensure the cancel request has been
   received and processed.


RETURN VALUE
============

``flux_kvs_lookup()`` and ``flux_kvs_lookupat()`` return a
``flux_future_t`` on success, or NULL on failure with errno set appropriately.

``flux_kvs_lookup_get()``, ``flux_kvs_lookup_get_unpack()``,
``flux_kvs_lookup_get_raw()``, ``flux_kvs_lookup_get_dir()``,
``flux_kvs_lookup_get_treeobj()``, ``flux_kvs_lookup_get_symlink()``,
and ``flux_kvs_lookup_cancel()`` return 0 on success, or -1 on failure with
errno set appropriately.

``flux_kvs_lookup_get_key()`` returns key on success, or NULL with errno
set to EINVAL if its future argument did not come from a KVS lookup.


ERRORS
======

EINVAL
   One of the arguments was invalid, or FLUX_KVS_READLINK was used but
   the key does not refer to a symlink.

ENOMEM
   Out of memory.

ENOENT
   An unknown key was requested.

ENOTDIR
   FLUX_KVS_READDIR flag was set and key does NOT point to a directory.

EISDIR
   FLUX_KVS_READDIR flag was NOT set and key points to a directory.

EPROTO
   A request or response was malformed.

EFBIG; ENOSYS
   The KVS module is not loaded.

ENOTSUP
   An unknown namespace was requested or namespace was deleted.

ENODATA
   A stream of responses requested with FLUX_KVS_WATCH was terminated
   with ``flux_kvs_lookup_cancel()``.

EPERM
   The user does not have instance owner capability, and a lookup was attempted
   against a KVS namespace owned by another user.


RESOURCES
=========

Flux: http://flux-framework.org

RFC 11: Key Value Store Tree Object Format v1: https://flux-framework.readthedocs.io/projects/flux-rfc/en/latest/spec_11.html

