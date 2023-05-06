===============
idset_encode(3)
===============


SYNOPSIS
========

::

   #include <flux/idset.h>

::

   char *idset_encode (const struct idset *idset, int flags);

::

   struct idset *idset_decode (const char *s);

::

   struct idset *idset_ndecode (const char *s, size_t len);


USAGE
=====

cc [flags] files -lflux-idset [libraries]


DESCRIPTION
===========

Refer to :man3:`idset_create` for a general description of idsets.

``idset_encode()`` creates a string from *idset*. The string contains
a comma-separated list of ids, potentially modified by *flags*
(see FLAGS below).

``idset_decode()`` creates an idset from a string *s*. The string may
have been produced by ``idset_encode()``. It must consist of comma-separated
non-negative integer ids, and may also contain hyphenated ranges.
If enclosed in square brackets, the brackets are ignored. Some examples
of valid input strings are:

::

   1,2,5,4

::

   1-4,7,9-10

::

   42

::

   [99-101]

``idset_ndecode()`` creates an idset from a sub-string *s* defined by
length *len*.


FLAGS
=====

IDSET_FLAG_BRACKETS
   Valid for ``idset_encode()`` only. If set, the encoded string will be
   enclosed in brackets, unless the idset is a singleton (contains only
   one id).

IDSET_FLAG_RANGE
   Valid for ``idset_encode()`` only. If set, any consecutive ids are
   compressed into hyphenated ranges in the encoded string.


RETURN VALUE
============

``idset_decode()`` and ``idset_ndecode()`` return idset on success which must
be freed with :man3:`idset_destroy`. On error, NULL is returned with errno set.

``idset_encode()`` returns a string on success which must be freed
with ``free()``. On error, NULL is returned with errno set.


ERRORS
======

EINVAL
   One or more arguments were invalid.

ENOMEM
   Out of memory.


RESOURCES
=========

Flux: http://flux-framework.org

RFC 22: Idset String Representation: https://flux-framework.readthedocs.io/projects/flux-rfc/en/latest/spec_22.html


SEE ALSO
========

:man3:`idset_create`
