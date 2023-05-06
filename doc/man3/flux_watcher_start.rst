=====================
flux_watcher_start(3)
=====================


SYNOPSIS
========

void flux_watcher_start (flux_watcher_t \*w);

void flux_watcher_stop (flux_watcher_t \*w);

void flux_watcher_destroy (flux_watcher_t \*w);

double flux_watcher_next_wakeup (flux_watcher_t \*w);


DESCRIPTION
===========

``flux_watcher_start()`` activates a flux_watcher_t object *w* so that it
can receive events. If *w* is already active, the call has no effect.
This may be called from within a flux_watcher_f callback.

``flux_watcher_stop()`` deactivates a flux_watcher_t object *w* so that it
stops receiving events. If *w* is already inactive, the call has no effect.
This may be called from within a flux_watcher_f callback.

``flux_watcher_destroy()`` destroys a flux_watcher_t object *w*,
after stopping it. It is not safe to destroy a watcher object within a
flux_watcher_f callback.

``flux_watcher_next_wakeup()`` returns the absolute time that the watcher
is supposed to trigger next. This function only works for *timer* and
*periodic* watchers, and will return a value less than zero with errno
set to ``EINVAL`` otherwise.


RESOURCES
=========

Flux: http://flux-framework.org


SEE ALSO
========

:man3:`flux_reactor_create`
