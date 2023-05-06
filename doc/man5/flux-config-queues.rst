=====================
flux-config-queues(5)
=====================


DESCRIPTION
===========

The ``queues`` table configures job queues, as described in RFC 33.
Normally, Flux has a single anonymous queue, but when queues are configured,
all queues are named, and a job submitted without a queue name is rejected
unless a default queue is configured.

Each key in the ``queues`` table is a queue name, whose value is a table
consisting of the following optional keys:

requires (array)
   A list of resource property names that selects a subset of the total
   configured resources for the queue.  Each property name is a string, as
   defined in RFC 20.  If multiple properties are listed, they are combined
   in a logical *and* operation.  Properties are attached to resources in the
   ``resource`` table as described in :man5:`flux-config-resource`.

policy (table)
   A policy table as described in :man5:`flux-config-policy` that overrides
   the general system policy for jobs submitted to this queue.

A default queue name may be configured by setting
``policy.jobspec.defaults.system.queue`` as described in
:man5:`flux-config-policy`.


EXAMPLE
=======

::

   [[resource.config]]
   hosts = test[0-7]
   properties = ["debug"]

   [[resource.config]]
   hosts = test[8-127]
   properties = ["batch"]

   [queues.debug]
   policy.limits.duration = "30m"
   requires = [ "debug" ]

   [queues.batch]
   policy.limits.duration = "8h"
   requires = [ "batch" ]

   [policy.jobspec.defaults.system]
   queue = "batch"

   [sched-fluxion-qmanager]
   queues = "batch debug"
   queue-policy-per-queue="batch:easy debug:fcfs"

   [sched-fluxion-resource]
   match-policy = "lonodex"
   match-format = "rv1_nosched"


CAVEATS
=======

Queue resources should not overlap.


RESOURCES
=========

Flux: http://flux-framework.org

RFC 20: Resource Set Specification Version 1: https://flux-framework.readthedocs.io/projects/flux-rfc/en/latest/spec_20.html

RFC 33: Flux Job Queues: https://flux-framework.readthedocs.io/projects/flux-rfc/en/latest/spec_33.html

SEE ALSO
========

:man5:`flux-config`, :man5:`flux-config-policy`, :man5:`flux-config-resource`
