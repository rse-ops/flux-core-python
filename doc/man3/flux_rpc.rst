===========
flux_rpc(3)
===========


SYNOPSIS
========

::

   #include <flux/core.h>

::

   flux_future_t *flux_rpc (flux_t *h, const char *topic,
                            const char *s,
                            uint32_t nodeid, int flags);

::

   flux_future_t *flux_rpc_pack (flux_t *h, const char *topic,
                                 uint32_t nodeid, int flags,
                                 const char *fmt, ...);

::

   flux_future_t *flux_rpc_raw (flux_t *h, const char *topic,
                                const void *data, int len,
                                uint32_t nodeid, int flags);

::

   flux_future_t *flux_rpc_message (flux_t *h,
                                    const flux_msg_t *msg,
                                    uint32_t nodeid, int flags);

::

   int flux_rpc_get (flux_future_t *f, const char **s);

::

   int flux_rpc_get_unpack (flux_future_t *f, const char *fmt, ...);

::

   int flux_rpc_get_raw (flux_future_t *f,
                         const void **data, int *len);

::

   uint32_t flux_rpc_get_matchtag (flux_future_t *f);

::

   uint32_t flux_rpc_get_nodeid (flux_future_t *f);


DESCRIPTION
===========

A remote procedure call (RPC) consists of a matched request and
response message exchanged with a Flux service. ``flux_rpc()``,
``flux_rpc_pack()``, and ``flux_rpc_raw()`` encode and send a request message
via Flux broker handle *h* to a Flux service identified by *topic*
and *nodeid*. A ``flux_future_t`` object is returned which acts as a handle
for synchronization and a container for the response message which in
turn contains the RPC result.

A lower-level variant of ``flux_rpc()``, ``flux_rpc_message()`` accepts a
pre-created request message, assigning *nodeid* and matchtag according
to *flags*.

:man3:`flux_future_then` may be used to register a reactor callback
(continuation) to be called once the response has been received.
:man3:`flux_future_wait_for` may be used to block until the
response has been received. Both accept an optional timeout.

``flux_rpc_get()``, ``flux_rpc_get_unpack()``, and ``flux_rpc_get_raw()``
decode the RPC result. Internally, they call ``flux_future_get()``
to access the response message stored in the future. If the response
message has not yet been received, these functions block until it is,
or an error occurs.

``flux_rpc_get_matchtag()`` and ``flux_rpc_get_nodeid()`` are accessors
which allow access to the RPC matchtag and target nodeid from the
future returned from ``flux_rpc(3)``.


REQUEST OPTIONS
===============

The request message is encoded and sent with or without a payload
using one of the three ``flux_rpc()`` variants.

``flux_rpc()`` attaches *s*, a NULL terminated string, as request
payload. If NULL, the request is encoded without a payload.

``flux_rpc_pack()`` attaches a JSON payload encoded as a NULL terminated
string using Jansson ``json_pack()`` style arguments (see below).

``flux_rpc_raw()`` attaches a raw payload *data* of length *len*, in bytes.
If *data* is NULL, the request is encoded without a payload.

*nodeid* affects request routing, and must be set to one of the following
values:

FLUX_NODEID_ANY
   The request is routed to the first matching service instance.

FLUX_NODEID_UPSTREAM
   The request is routed to the first matching service instance,
   skipping over the sending rank.

integer
   The request is routed to a specific rank.

*flags* may be zero or:

FLUX_RPC_NORESPONSE
   No response is expected. The request will not be assigned a matchtag,
   and the returned flux_future_t is immediately fulfilled, and may simply
   be destroyed.

FLUX_RPC_STREAMING
   The RPC is for a service that may send zero or more non-error responses,
   and a final error response. ENODATA should be interpreted as a non-error
   end-of-stream sentinel.


RESPONSE OPTIONS
================

The response message is stored in the future when the future is fulfilled.
At that time it is decoded with :man3:`flux_response_decode`. If it cannot
be decoded, or if the service returned an error, the future is fulfilled
with an error. Otherwise it is fulfilled with the response message.
If there was an error, ``flux_future_get()`` or the ``flux_rpc_get()`` variants
return an error.

``flux_rpc_get()`` sets *s* (if non-NULL) to the NULL-terminated string
payload contained in the RPC response. If there was no payload, *s*
is set to NULL.

``flux_rpc_get_unpack()`` decodes the NULL-terminated string payload as JSON
using Jansson ``json_unpack()`` style arguments (see below). It is an error
if there is no payload, or if the payload is not JSON.

``flux_rpc_get_raw()`` assigns the raw payload of the RPC response message
to *data* and its length to *len*. If there is no payload, this function
will fail.


PREMATURE DESTRUCTION
=====================

If a regular RPC future is destroyed before its response is received,
the matchtag allocated to it is not immediately returned to the pool
for reuse. If an unclaimed response subsequently arrives with that
matchtag, it is returned to the pool then.

If a **streaming** RPC future is destroyed before its terminating response
is received, its matchtag is only returned to the pool when an unclaimed
**error** response is received. Non-error responses are ignored.

It is essential that services which return multiple responses verify that
requests were made with the FLUX_RPC_STREAMING flag and return an immediate
EPROTO error if they were not. See :man3:`flux_respond`.


CANCELLATION
============

Flux RFC 6 does not currently specify a cancellation protocol for an
individual RPC, but does stipulate that an RPC may be canceled if a disconnect
message is received, as is automatically generated by the local connector
upon client disconnection.

.. include:: JSON_PACK.rst

.. include:: JSON_UNPACK.rst


RETURN VALUE
============

``flux_rpc()``, ``flux_rpc_pack()``, and ``flux_rpc_raw()`` return a flux_future_t
object on success. On error, NULL is returned, and errno is set appropriately.

``flux_rpc_get()``, ``flux_rpc_get_unpack()``, and ``flux_rpc_get_raw()`` return
zero on success. On error, -1 is returned, and errno is set appropriately.

``flux_rpc_get_matchtag()`` returns the matchtag allocated to the particular
RPC request, or ``FLUX_MATCHTAG_NONE`` if no matchtag was allocated (e.g. no
response is expected), or the future argument does not correspond to an RPC.

``flux_rpc_get_nodeid()`` returns the original ``nodeid`` target of the
``flux_rpc()`` request, including if the RPC was targeted to
``FLUX_NODEID_ANY`` or ``FLUX_NODEID_UPSTREAM``.

ERRORS
======

ENOSYS
   Service is not available (misspelled topic string, module not loaded, etc),
   or flux_t handle has no send operation.

EINVAL
   Some arguments were invalid.

EPROTO
   A request was malformed, the FLUX_RPC_STREAMING flag was
   omitted on a request to a service that may send multiple responses,
   or other protocol error occurred.


EXAMPLES
========

This example performs a synchronous RPC with the broker's "attr.get"
service to obtain the broker's rank.

.. literalinclude:: trpc.c

This example registers a continuation to do the same thing asynchronously.

.. literalinclude:: trpc_then.c


RESOURCES
=========

Flux: http://flux-framework.org

RFC 6: Flux Remote Procedure Call Protocol: https://flux-framework.readthedocs.io/projects/flux-rfc/en/latest/spec_6.html


SEE ALSO
========

:man3:`flux_future_get`, :man3:`flux_respond`
