[![ci](https://github.com/flux-framework/flux-core/workflows/ci/badge.svg)](https://github.com/flux-framework/flux-core/actions?query=workflow%3A.github%2Fworkflows%2Fmain.yml)
[![codecov](https://codecov.io/gh/flux-framework/flux-core/branch/master/graph/badge.svg)](https://codecov.io/gh/flux-framework/flux-core)

See our [Online Documentation](https://flux-framework.readthedocs.io)!

_NOTE: the github issue tracker is the primary way to communicate
with Flux developers._


### flux-core

flux-core implements the lowest level services and interfaces for the Flux
resource manager framework.  It is intended to be the first building block
used in the construction of a site-composed Flux resource manager.  Other
building blocks are also in development under the
[flux-framework github organization](https://github.com/flux-framework),
including a workload [scheduler](https://github.com/flux-framework/flux-sched).

Framework projects use the C4 development model pioneered in
the ZeroMQ project and forked as
[Flux RFC 1](https://flux-framework.rtfd.io/projects/flux-rfc/en/latest/spec_1.html).
Flux licensing and collaboration plans are described in
[Flux RFC 2](https://flux-framework.rtfd.io/projects/flux-rfc/en/latest/spec_2.html).
Protocols and API's used in Flux will be documented as Flux RFC's.

#### Build Requirements

For convenience, scripts that install flux-core's build dependencies
are available for [redhat](scripts/install-deps-rpm.sh) and
[debian](scripts/install-deps-deb.sh) distros.

##### Building from Source
```
./autogen.sh   # skip if building from a release tarball
./configure
make
make check
```

##### VSCode Dev Containers

If you use VSCode we have a dev container and [instructions](vscode.md).

#### Starting Flux

A Flux instance is composed of a set of `flux-broker` processes running as
a parallel job and can be started by most launchers that can start MPI jobs.
Doing so for a single user does not require administrator privilege.
To start a Flux instance (size = 8) on the local node for testing, use
flux's built-in test launcher:
```
src/cmd/flux start --test-size=8
```
A shell is spawned in which Flux commands can be executed.  When the shell
exits, Flux exits.

For more information on starting Flux in various environments and using it,
please refer to our [docs](https://flux-framework.readthedocs.io) pages.

#### Release

SPDX-License-Identifier: LGPL-3.0

LLNL-CODE-764420
