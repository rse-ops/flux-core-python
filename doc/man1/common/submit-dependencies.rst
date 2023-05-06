DEPENDENCIES
============

.. note::
   Flux supports a simple but powerful job dependency specification in jobspec.
   See Flux Framework RFC 26 for more detailed information about the generic
   dependency specification.

Dependencies may be specified on the command line using the following options:

**--dependency=URI**
   Specify a dependency of the submitted job using RFC 26 dependency URI
   format. The URI format is **SCHEME:VALUE[?key=val[&key=val...]]**.
   The URI will be converted into RFC 26 JSON object form and appended to
   the jobspec ``attributes.system.dependencies`` array. If the current
   Flux instance does not support dependency scheme *SCHEME*, then the
   submitted job will be rejected with an error message indicating this
   fact.

   The ``--dependency`` option may be specified multiple times. Each use
   appends a new dependency object to the ``attributes.system.dependencies``
   array.

The following dependency schemes are built-in:

.. note::
   The ``after*`` dependency schemes listed below all require that the
   target JOBID be currently active or in the job manager's inactive job
   cache. If a target JOBID has been purged by the time the dependent job
   has been submitted, then the submission will be rejected with an error
   that the target job cannot be found.

after:JOBID
   This dependency is satisfied after JOBID starts.

afterany:JOBID
   This dependency is satisfied after JOBID enters the INACTIVE state,
   regardless of the result

afterok:JOBID
   This dependency is satisfied after JOBID enters the INACTIVE state
   with a successful result.

afternotok:JOBID
   This dependency is satisfied after JOBID enters the INACTIVE state
   with an unsuccessful result.

begin-time:TIMESTAMP
   This dependency is satisfied after TIMESTAMP, which is specified in
   floating point seconds since the UNIX epoch. See the ``--begin-time``
   option below for a more user-friendly interface to the ``begin-time``
   dependency.

In any of the above ``after*`` cases, if it is determined that the
dependency cannot be satisfied (e.g. a job fails due to an exception
with afterok), then a fatal exception of type=dependency is raised
on the current job.

