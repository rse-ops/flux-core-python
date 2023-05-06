=====================
flux_future_create(3)
=====================


SYNOPSIS
========

::

   #include <flux/core.h>

::

   typedef void (*flux_future_init_f)(flux_future_t *f,
                                      flux_reactor_t *r, void *arg);

::

   flux_future_t *flux_future_create (flux_future_init_f cb, void *arg);

::

   void flux_future_fulfill (flux_future_t *f,
                             void *result, flux_free_f free_fn);

::

   void flux_future_fulfill_error (flux_future_t *f, int errnum,
                                   const char *errstr);

::

   void flux_future_fulfill_with (flux_future_t *f, flux_future_t *p);

::

   void flux_future_fatal_error (flux_future_t *f, int errnum,
                                 const char *errstr);

::

   void *flux_future_aux_get (flux_future_t *f, const char *name);

::

   int flux_future_aux_set (flux_future_t *f, const char *name,
                            void *aux, flux_free_f destroy);

::

   void flux_future_set_reactor (flux_future_t *f, flux_t *h);

::

   flux_reactor_t *flux_future_get_reactor (flux_future_t *f);

::

   void flux_future_set_flux (flux_future_t *f, flux_t *h);

::

   flux_t *flux_future_get_flux (flux_future_t *f);


DESCRIPTION
===========

See :man3:`flux_future_get` for general functions that operate on futures.
This page covers functions primarily used when building classes that
return futures.

A Flux future represents some activity that may be completed with reactor
watchers and/or message handlers. It is intended to be returned by other
classes as a handle for synchronization and a container for results.
This page describes the future interfaces used by such classes.

A class that returns a future usually provides a creation function
that internally calls ``flux_future_create()``, and may provide functions
to access class-specific result(s), that internally call ``flux_future_get()``.
The create function internally registers a *flux_future_init_f*
function that is called lazily by the future implementation to perform
class-specific reactor setup, such as installing watchers and message
handlers.

``flux_future_create()`` creates a future and registers the
class-specific initialization callback *cb*, and an opaque argument
*arg* that will be passed to *cb*. The purpose of the initialization
callback is to set up class-specific watchers on a reactor obtained
with ``flux_future_get_reactor()``, or message handlers on a flux_t
handle obtained with ``flux_future_get_flux()``, or both.
``flux_future_get_reactor()`` and ``flux_future_get_flux()`` return
different results depending on whether the initialization callback is
triggered by a user calling ``flux_future_then()`` or
``flux_future_wait_for()``. The function may be triggered in one or
both contexts, at most once for each. The watchers or message
handlers must eventually call ``flux_future_fulfill()``,
``flux_future_fulfill_error()``, or ``flux_future_fatal_error()`` to
fulfill the future. See REACTOR CONTEXTS below for more information.

``flux_future_fulfill()`` fulfills the future, assigning an opaque
*result* value with optional destructor *free_fn* to the future.
A NULL *result* is valid and also fulfills the future. The *result*
is contained within the future and can be accessed with ``flux_future_get()``
as needed until the future is destroyed.

``flux_future_fulfill_error()`` fulfills the future, assigning an
*errnum* value and an optional error string. After the future is
fulfilled with an error, ``flux_future_get()`` will return -1 with errno
set to *errnum*.

``flux_future_fulfill_with()`` fulfills the target future *f* using a
fulfilled future *p*. This function copies the pending result or error
from *p* into *f*, and adds read-only access to the *aux* items for *p*
from *f*. This ensures that any ``get`` method which requires *aux* items
for *p* will work with *f*. This function takes a reference to the source
future *p*, so it safe to call ``flux_future_destroy (p)`` after this call.
``flux_future_fulfill_with()`` returns -1 on error with *errno*
set on failure.

``flux_future_fulfill()``, ``flux_future_fulfill_with()``, and
``flux_future_fulfill_error()`` can be called multiple times to queue
multiple results or errors. When callers access future results via
``flux_future_get()``, results or errors will be returned in FIFO order.
It is an error to call ``flux_future_fulfill_with()`` multiple times on
the same target future *f* with a different source future *p*.

``flux_future_fatal_error()`` fulfills the future, assigning an *errnum*
value and an optional error string. Unlike
``flux_future_fulfill_error()`` this fulfillment can only be called once
and takes precedence over all other fulfillments. It is used for
catastrophic error paths in future fulfillment.

``flux_future_aux_set()`` attaches application-specific data
to the parent object *f*. It stores data *aux* by key *name*,
with optional destructor *destroy*. The destructor, if non-NULL,
is called when the parent object is destroyed, or when
*key* is overwritten by a new value. If *aux* is NULL,
the destructor for a previous value, if any is called,
but no new value is stored. If *name* is NULL,
*aux* is stored anonymously.

``flux_future_aux_get()`` retrieves application-specific data
by *name*. If the data was stored anonymously, it
cannot be retrieved.

Names beginning with "flux::" are reserved for internal use.

``flux_future_set_reactor()`` may be used to associate a Flux reactor
with a future. The reactor (or a temporary one, depending on the context)
may be retrieved using ``flux_future_get_reactor()``.

``flux_future_set_flux()`` may be used to associate a Flux broker handle
with a future. The handle (or a clone associated with a temporary reactor,
depending on the context) may be retrieved using ``flux_future_get_flux()``.

Futures may "contain" other futures, to arbitrary depth. That is, an
init callback may create futures and use their continuations to fulfill
the containing future in the same manner as reactor watchers and message
handlers.


REACTOR CONTEXTS
================

Internally, a future can operate in two reactor contexts. The initialization
callback may be called in either or both contexts, depending on which
synchronization functions are called by the user. ``flux_future_get_reactor()``
and ``flux_future_get_flux()`` return a result that depends on which context
they are called from.

When the user calls ``flux_future_then()``, this triggers a call to the
initialization callback. The callback would typically call
``flux_future_get_reactor()`` and/or ``flux_future_get_flux()`` to obtain the
reactor or flux_t handle to be used to set up watchers or message handlers.
In this context, the reactor or flux_t handle are exactly the ones passed
to ``flux_future_set_reactor()`` and ``flux_future_set_flux()``.

When the user calls ``flux_future_wait_for()``, this triggers the creation
of a temporary reactor, then a call to the initialization callback.
The temporary reactor allows these functions to wait *only* for the future's
events, without allowing unrelated watchers registered in the main reactor
to run, which might complicate the application's control flow. In this
context, ``flux_future_get_reactor()`` returns the temporary reactor, not
the one passed in with ``flux_future_set_reactor()``. ``flux_future_get_flux()``
returns a temporary flux_t handle cloned from the one passed to
``flux_future_set_flux()``, and associated with the temporary reactor.
After the internal reactor returns, any messages unmatched by the dispatcher
on the cloned handle are requeued in the main flux_t handle with
``flux_dispatch_requeue()``.

Since the init callback may be made in either reactor context (at most once
each), and is unaware of which context that is, it should take care when
managing any context-specific state not to overwrite the state from a prior
call. The ability to attach objects with destructors anonymously to the future
with ``flux_future_aux_set()`` may be useful for managing the life cycle
of reactor watchers and message handlers created by init callbacks.


RETURN VALUE
============

``flux_future_create()`` returns a future on success. On error, NULL is
returned and errno is set appropriately.

``flux_future_aux_set()`` returns zero on success. On error, -1 is
returned and errno is set appropriately.

``flux_future_aux_get()`` returns the requested object on success. On
error, NULL is returned and errno is set appropriately.

``flux_future_get_flux()`` returns a flux_t handle on success. On error,
NULL is returned and errno is set appropriately.

``flux_future_get_reactor()`` returns a flux_reactor_t on success. On error,
NULL is returned and errno is set appropriately.

``flux_future_fulfill_with()`` returns zero on success. On error, -1 is
returned with errno set to EINVAL if either *f* or *p* is NULL, or
*f* and *p* are the same, EAGAIN if the future *p* is not ready, or
EEXIST if the function is called multiple times with different *p*.


ERRORS
======

ENOMEM
   Out of memory.

EINVAL
   Invalid argument.

ENOENT
   The requested object is not found.

EAGAIN
   The requested operation is not ready. For ``flux_future_fulfill_with()``,
   the target future *p* is not fulfilled.

EEXIST
   ``flux_future_fulfill_with()`` was called multiple times with a different
   target future *p*.


RESOURCES
=========

Flux: http://flux-framework.org


SEE ALSO
========

:man3:`flux_future_get`, :man3:`flux_clone`
