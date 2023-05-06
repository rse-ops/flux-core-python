==============================
flux_future_wait_all_create(3)
==============================


SYNOPSIS
========

::

   #include <flux/core.h>

::

   flux_future_t *flux_future_wait_all_create (void);
   flux_future_t *flux_future_wait_any_create (void);

::

   int flux_future_push (flux_future_t *cf, const char *name, flux_future_t *f);

::

   const char *flux_future_first_child (flux_future_t *cf);
   const char *flux_future_next_child (flux_future_t *cf);
   flux_future_t *flux_future_get_child (flux_future_t *cf, const char *name);


DESCRIPTION
===========

See :man3:`flux_future_get` for general functions that operate on futures,
and :man3:`flux_future_create` for a description of the ``flux_future_t``
base type. This page covers functions used for composing futures into
composite types using containers that allow waiting on all or any of a
set of child futures.

``flux_future_wait_all_create(3)`` creates a future that is an empty
container for other futures, which can subsequently be pushed into
the container using ``flux_future_push(3)``. The returned future will
be automatically fulfilled when **all** children futures have been
fulfilled. The caller may then use ``flux_future_first_child(3)``,
``flux_future_next_child(3)``, and/or ``flux_future_get_child(3)`` and
expect that ``flux_future_get(3)`` will not block for any of these child
futures. This function is useful to synchronize on a series of futures
that may be run in parallel.

``flux_future_wait_any_create(3)`` creates a composite future that will be
fulfilled once **any** one of its children are fulfilled. Once the composite
future is fulfilled, the caller will need to traverse the child futures
to determine which was fulfilled. This function is useful to synchronize
on work where any one of several results is sufficient to continue.

``flux_future_push(3)`` places a new child future ``f`` into a future
composite created by either ``flux_future_wait_all_create(3)`` or
``flux_future_wait_any_create(3)``. A ``name`` is provided for the child so
that the child future can be easily differentiated from other futures
inside the container once the composite future is fulfilled.

Once a ``flux_future_t`` is pushed onto a composite future with
``flux_future_push(3)``, the memory for the child future is "adopted" by
the new parent. Thus, calling ``flux_future_destroy(3)`` on the parent
composite will destroy all children. Therefore, child futures that
have been the target of ``flux_future_push(3)`` should **not** have
flux_future_destroy(3)\` called upon them to avoid double-free.

``flux_future_first_child(3)`` and ``flux_future_next_child(3)`` are used to
iterate over child future names in a composite future created with either
``flux_future_wait_all_create(3)`` or ``flux_future_wait_any_create(3)``. The
``flux_future_t`` corresponding to the returned *name* can be then
fetched with ``flux_future_get_child(3)``. ``flux_future_next_child`` will
return a ``NULL`` once all children have been iterated.

``flux_future_get_child(3)`` retrieves a child future from a composite
by name.


RETURN VALUE
============

``flux_future_wait_any_create()`` and ``flux_future_wait_all_create()`` return
a future on success. On error, NULL is returned and errno is set appropriately.

``flux_future_push()`` returns zero on success. On error, -1 is
returned and errno is set appropriately.

``flux_future_first_child()`` returns the name of the first child future in
the targeted composite in no given order. If the composite is empty,
a NULL is returned.

``flux_future_next_child()`` returns the name of the next child future in the
targeted composite in no given order. If the last child has already been
returned then this function returns NULL.

``flux_future_get_child()`` returns a ``flux_future_t`` corresponding to the
child future with the supplied string ``name`` parameter. If no future with
that name is a child of the composite, then the function returns NULL.


ERRORS
======

ENOMEM
   Out of memory.

EINVAL
   Invalid argument.

ENOENT
   The requested object is not found.


RESOURCES
=========

Flux: http://flux-framework.org


SEE ALSO
========

:man3:`flux_future_get`, :man3:`flux_future_create`
