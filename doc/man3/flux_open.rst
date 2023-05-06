============
flux_open(3)
============


SYNOPSIS
========

::

   #include <flux/core.h>

   flux_t *flux_open (const char *uri, int flags);

   flux_t *flux_open_ex (const char *uri, int flags, flux_error_t *error);

   void flux_close (flux_t *h);

   flux_t *flux_clone (flux_t *h);

   int flux_reconnect (flux_t *h);


DESCRIPTION
===========

``flux_open()`` and ``flux_open_ex()`` create a ``flux_t`` handle, used
to communicate with the Flux message broker. ``flux_open_ex()`` takes an
optional pointer to a ``flux_error_t`` structure which, when non-NULL, will
be used to store any errors which may have otherwise gone to ``stderr``.

The *uri* scheme (before "://") specifies the "connector" that will be used
to establish the connection. The *uri* path (after "://") is parsed by the
connector. If *uri* is NULL, the value of $FLUX_URI is used.  If $FLUX_URI is
not set, a compiled-in default URI is used.

*flags* is the logical "or" of zero or more of the following flags:

FLUX_O_TRACE
   Dumps message trace to stderr.

FLUX_O_CLONE
   Used internally by ``flux_clone()`` (see below).

FLUX_O_MATCHDEBUG
   Prints diagnostic to stderr when matchtags are leaked, for example when
   a streaming RPC is destroyed without receiving a error response as
   end-of-stream, or a regular RPC is destroyed without being fulfilled at all.

FLUX_O_NONBLOCK
   The :man3:`flux_send` and :man3:`flux_recv` functions should never block.

FLUX_O_TEST_NOSUB
   Make :man3:`flux_event_subscribe` and ``flux_event_unsubscribe()`` no-ops.
   This may be useful in specialized situations with the ``loop://`` connector,
   where no message handler is available to service subscription RPCs.

FLUX_O_RPCTRACK
   Track pending RPCs so that they can receive automatic ECONNRESET failure
   responses if the broker connection is re-established with
   ``flux_reconnect()``.  Tracking incurs a small overhead.  This flag can
   only be specified with ``flux_open()``, not :man3:`flux_flags_set`.

``flux_reconnect()`` may be called from a communications error callback
registered with :man3:`flux_comms_error_set`.  The current connection is
closed and a new one is established with a new UUID.  Since responses addressed
to the old UUID will not be routed to the new connection, RPCs that are pending
before ``flux_reconnect()`` remain pending indefinitely without FLUX_O_RPCTRACK.
After a successful reconnect, the following additional steps may be needed
before a client can continue normal operation:

- Wait until the broker has entered RUN state by making an RPC to ``state_machine.wait``
- Restore service registrations.
- Restore event subscriptions.

``flux_clone()`` creates another reference to a ``flux_t`` handle that is
identical to the original in all respects except that it does not inherit
a copy of the original handle's "aux" hash, or its reactor and message
dispatcher references. By creating a clone, and calling ``flux_set_reactor()``
on it, one can create message handlers on the cloned handle that run on a
different reactor than the one associated with the original handle.

``flux_close()`` destroys a ``flux_t`` handle, closing its connection with
the Flux message broker.


RETURN VALUE
============

``flux_open()`` and ``flux_clone()`` return a ``flux_t`` handle on success.
On error, NULL is returned, with errno set.


ERRORS
======

EINVAL
   One or more arguments was invalid.

ENOMEM
   Out of memory.


EXAMPLES
========

This example opens the Flux broker using the default connector
and path, requests the broker rank, and finally closes the broker handle.

.. literalinclude:: topen.c


RESOURCES
=========

Flux: http://flux-framework.org


SEE ALSO
========

:man1:`flux-uri`, :man3:`flux_comms_error_set`
