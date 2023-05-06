.. flux-help-description: Manage/query Flux configuration

==============
flux-config(1)
==============


SYNOPSIS
========

**flux** **config** **reload**

**flux** **config** **load** [*PATH*]

**flux** **config** **get** [*OPTIONS*] [*NAME*]

**flux** **config** **builtin** [*NAME*]


DESCRIPTION
===========

``flux config reload`` tells :man1:`flux-broker` to reload its TOML
configuration.  Further details of Flux configuration are described in
:man5:`flux-config`, including some caveats on configuration updates.
On Flux instances started with :linux:man1:`systemd`, ``systemctl reload flux``
invokes this command.  This command is restricted to the instance owner.

``flux config get`` queries the TOML configuration for a given Flux broker.
if *NAME* is unspecified, it dumps the entire configuration object.  Otherwise,
*NAME* is expected to be a period-delimited path name representing a TOML key.
Return values are printed in string-encoded JSON form, except for string values,
which are printed without quotes to simplify their use in shell scripts.

``flux config builtin`` prints compiled-in Flux configuration values.
See BUILTIN VALUES below for a list of builtin
configuration key names.  This command is available to all users.

.. note::
   ``flux config get`` and ``flux config builtin`` refer to disjoint key
   namespaces.  Flux behavior is determined by a combination of these values,
   :man7:`flux-broker-attributes`, and other factors.  This disjoint
   configuration scheme is subject to change in future releases of Flux.

.. note::
   ``flux config builtin`` uses a heuristic to determine if :man1:`flux`
   was run from the flux-core source tree, and substitutes source tree
   specific values if found to be in tree.  This enables Flux testing without
   requiring installation.

``flux config load`` replaces the current config with an object read from
standard input (JSON or TOML), or from ``*.toml`` in *PATH*, if specified.

GET SUBCOMMAND OPTIONS
======================

**-h, --help**
   Display subcommand help.

**-d, --default**\ =\ *VALUE*
   Substitute *VALUE* if *NAME* is not set in the configuration, and exit
   with a return code of zero.

**-q, --quiet**
   Suppress printing of errors if *NAME* is not set and *--default* was not
   specified.  This may be convenient to avoid needing to redirect standard
   error in a shell script.

**-t, --type**\ =\ *TYPE*
   Require that the value has the specified type, or exit with a nonzero exit
   code.  Valid types are *string*, *integer*, *real*, *boolean*, *object*, and
   *array*.  In addition, types of *fsd*, *fsd-integer*, and *fsd-real* ensure
   that a value is a both a string and valid Flux Standard Duration.
   *fsd* prints the value in its human-readable, string form. *fsd-integer*
   and *fsd-real* print the value in integer and real seconds, respectively.


BUILTIN VALUES
==============

The following configuration keys may be printed with ``flux config builtin``:

**rc1_path**
   The rc1 script path used by :man1:`flux-broker`, unless overridden by
   the ``broker.rc1_path`` broker attribute.

**rc3_path**
   The rc3 script path used by :man1:`flux-broker`, unless overridden by
   the ``broker.rc1_path`` broker attribute.

**shell_path**
   The path to the :man1:`flux-shell` executable used by the exec service.

**shell_pluginpath**
   The search path used by :man1:`flux-shell` to locate plugins, unless
   overridden by setting the ``conf.shell_pluginpath`` broker attribute.

**shell_initrc**
   The initrc script path used by :man1:`flux-shell`, unless overridden by
   setting the ``conf.shell_pluginpath`` broker attribute.

**jobtap_pluginpath**
   The search path used by the job manager to locate
   :man7:`flux-jobtap-plugins`.

**rundir**
   The configured ``${runstatedir}/flux`` directory.

**bindir**
   The configured ``${libexecdir/flux/cmd`` directory.

**lua_cpath_add**
   Consulted by :man1:`flux` when setting the LUA_CPATH environment variable.

**lua_path_add**
   Consulted by :man1:`flux` when setting the LUA_PATH environment variable.

**python_path**
   Consulted by :man1:`flux` when setting the PYTHONPATH environment variable.

**man_path**
   Consulted by :man1:`flux` when setting the MANPATH environment variable.

**exec_path**
   Consulted by :man1:`flux` when setting the FLUX_EXEC_PATH environment
   variable.

**connector_path**
   Consulted by :man1:`flux` when setting the FLUX_CONNECTOR_PATH environment
   variable.

**module_path**
   Consulted by :man1:`flux` when setting the FLUX_MODULE_PATH environment
   variable.

**pmi_library_path**
   Consulted by :man1:`flux` when setting the FLUX_PMI_LIBRARY_PATH environment
   variable.

**cmdhelp_pattern**
   Used by :man1:`flux` to generate a list of common commands when run without
   arguments.

**no_docs_path**


EXAMPLES
========

::

   $ flux config get --type=fsd-integer tbon.tcp_user_timeout
   60


RESOURCES
=========

Flux: http://flux-framework.org

RFC 23: Flux Standard Duration: https://flux-framework.readthedocs.io/projects/flux-rfc/en/latest/spec_23.html


SEE ALSO
========

:man5:`flux-config`, :man1:`flux-getattr`
