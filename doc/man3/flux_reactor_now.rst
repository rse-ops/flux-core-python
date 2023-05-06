===================
flux_watcher_now(3)
===================


SYNOPSIS
========

double flux_reactor_now (flux_reactor_t \*r);

void flux_reactor_now_update (flux_reactor_t \*r);

double flux_reactor_time (void);


DESCRIPTION
===========

``flux_reactor_now()`` returns the current reactor time, which is the time
the reactor began processing events. The time will not be updated until
the reactor runs out of events and wakes up again. This is a lighter
weight alternative to system calls when only coarse event timing is needed,
e.g. when all events processed in a given wakeup can be considered
simultaneous.

``flux_reactor_now_update()`` forces an update to reactor time.
This may be useful when the reactor has not run for a while and timing
calculations relative to reactor time need to be made, for example when
creating timer watchers.

``flux_reactor_time()`` returns the system time as a double.
Reactor time is a snapshot of ``flux_reactor_time()``.

Note: the Flux reactor is based on libev. For additional information
on the behavior of reactor time, refer to the libev documentation on
``ev_now`` and ``ev_now_update()``.


RESOURCES
=========

Flux: http://flux-framework.org

libev: http://software.schmorp.de/pkg/libev.html


SEE ALSO
========

:man3:`flux_reactor_create`
