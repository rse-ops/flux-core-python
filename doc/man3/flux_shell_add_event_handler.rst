===============================
flux_shell_add_event_handler(3)
===============================


SYNOPSIS
========

::

   #include <flux/shell.h>
   #include <errno.h>

::

   int flux_shell_add_event_handler (flux_shell_t *shell,
                                     const char *subtopic,
                                     flux_msg_handler_f cb,
                                     void *arg);


DESCRIPTION
===========

When the shell initializes, it subscribes to all events with the
substring ``shell-JOBID.``, where ``JOBID`` is the jobid under which the
shell is running. ``flux_shell_add_event_handler()`` registers a handler
to be run for a **subtopic** within the shell's event namespace, e.g.
registering a handler for ``subtopic`` ``"kill"`` will invoke the handler
``cb`` whenever an event named ``shell-JOBID.kill`` is generated.


RETURN VALUE
============

Returns -1 if ``shell``, ``shell->h``, ``subtopic`` or ``cb`` are NULL, or if
underlying calls to ``asprintf()`` or ``flux_msg_handler_create()`` fail.


ERRORS
======

EINVAL
   ``shell``, ``shell->h``, ``subtopic`` or ``cb`` are NULL.


RESOURCES
=========

Flux: http://flux-framework.org
