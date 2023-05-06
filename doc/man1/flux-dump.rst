.. flux-help-description: Write KVS snapshot to portable archive

============
flux-dump(1)
============


SYNOPSIS
========

**flux** **dump** [*OPTIONS*] *OUTFILE*


DESCRIPTION
===========

The ``flux-dump`` command writes a KVS snapshot to a portable archive format,
usually read by :man1:`flux-restore`.

The snapshot source is the primary namespace of the current KVS root by default.
If *--checkpoint* is specified, the snapshot source is the last KVS checkpoint
written to the content backing store.

The archive is a file path or *-* for standard output.  If standard output,
the format is POSIX *ustar* with no compression.  Otherwise the format is
determined by the file extension.  The list of valid extensions depends on the
version of :linux:man3:`libarchive` used to build Flux, but modern versions
support:

.tar
   POSIX *ustar* format, compatible with :linux:man1:`tar`.

.tgz, .tar.gz
   POSIX *ustar* format, compressed with :linux:man1:`gzip`.

.tar.bz2
   POSIX *ustar* format, compressed with :linux:man1:`bzip2`.

.tar.xz
   POSIX *ustar* format, compressed with :linux:man1:`xz`.

.zip
   ZIP archive, compatible with :linux:man1:`unzip`.

.cpio
   POSIX CPIO format, compatible with :linux:man1:`cpio`.

.iso
   ISO9660 CD image


OPTIONS
=======

**-h, --help**
   Summarize available options.

**-v, --verbose**
   List keys on stderr as they are dumped instead of a periodic count of
   dumped keys.

**-q, --quiet**
   Don't show periodic count of dumped keys on stderr.

**--checkpoint**
   Generate snapshot from the latest checkpoint written to the content
   backing store, instead of from the current KVS root.

**--no-cache**
   Bypass the broker content cache and interact directly with the backing
   store.  This may be slightly faster, depending on how frequently the same
   content blobs are referenced by multiple keys.


OTHER NOTES
===========

KVS commits are atomic and propagate to the root of the namespace.  Because of
this, when ``flux-dump`` archives a snapshot of a live system, it reflects one
point in time, and does not include any changes committed while the dump is
in progress.

Since ``flux-dump`` generates the archive by interacting directly with the
content store, the *--checkpoint* option may be used to dump the most recent
state of the KVS when the KVS module is not loaded.

Only regular values and symbolic links are dumped to the archive.  Directories
are not dumped as independent objects, so empty directories are omitted from
the archive.

KVS symbolic links represent the optional namespace component in the target
as a *NAME::* prefix.

The KVS path separator is converted to the UNIX-compatible slash so that the
archive can be unpacked into a file system if desired.

The modification time of files in the archive is set to the time that
``flux-dump`` is started if dumping the current KVS root, or to the timestamp
of the checkpoint if *--checkpoint* is used.

The owner and group of files in the archive are set to the credentials of the
user that ran ``flux-dump``.

The mode of files in the archive is set to 0644.


RESOURCES
=========

Flux: http://flux-framework.org

RFC 10: Content Storage Service: https://flux-framework.readthedocs.io/projects/flux-rfc/en/latest/spec_10.html

RFC 11: Key Value Store Tree Object Format v1: https://flux-framework.readthedocs.io/projects/flux-rfc/en/latest/spec_11.html




SEE ALSO
========

:man1:`flux-restore`, :man1:`flux-kvs`
