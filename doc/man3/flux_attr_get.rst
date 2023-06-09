================
flux_attr_get(3)
================


SYNOPSIS
========

::

   #include <flux/core.h>

::

   const char *flux_attr_get (flux_t *h, const char *name);

::

   int flux_attr_set (flux_t *h, const char *name, const char *val);


DESCRIPTION
===========

Flux broker attributes are both a simple, general-purpose key-value
store with scope limited to the local broker rank, and a method for the
broker to export information needed by Flux services and utilities.

``flux_attr_get()`` retrieves the value of the attribute *name*.

Attributes that have the broker tags as *immutable* are cached automatically
in the flux_t handle. For example, the "rank" attribute is a frequently
accessed attribute whose value never changes. It will be cached on the first
access and thereafter does not require an RPC to the broker to access.

``flux_attr_set()`` updates the value of attribute *name* to *val*.
If *name* is not currently a valid attribute, it is created.
If *val* is NULL, the attribute is unset.


RETURN VALUE
============

``flux_attr_get()`` returns the requested value on success. On error, NULL
is returned and errno is set appropriately.

``flux_attr_set()`` returns zero on success. On error, -1 is returned
and errno is set appropriately.


ERRORS
======

ENOENT
   The requested attribute is invalid or has a NULL value on the broker.

EINVAL
   Some arguments were invalid.

EPERM
   Set was attempted on an attribute that is not writable with the
   user's credentials.


RESOURCES
=========

Flux: http://flux-framework.org


SEE ALSO
========

:man1:`flux-getattr`, :man7:`flux-broker-attributes`,
