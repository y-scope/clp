# Building the package

This page describes the requirements and steps to build the CLP package. If you're looking for a
prebuilt version instead, check out the [releases](https://github.com/y-scope/clp/releases) page.

## Requirements

* An x86_64 Ubuntu 22.04 (Jammy) machine or container
  * At runtime, the CLP package uses an Ubuntu Jammy container, so we need to build in a matching
    environment.
  * It should be possible to build a package for a different environment, it just requires a some
    extra configuration.
* [Docker]
  * `containerd.io` >= 1.7.18
  * `docker-buildx-plugin` >= 0.15.1
  * `docker-ce` >= 27.0.3
  * `docker-ce-cli` >= 27.0.3
* Python 3.10 or newer
* python3-dev
* python3-venv (for the version of Python installed)
* [Task] 3.44.0
  * We pin the version to 3.44.0 due to [y-scope/clp#1352].
* [uv] >= 0.8

## Setup

Initialize the project

```shell
tools/scripts/deps-download/init.sh
```

Install CLP core's dependencies

```shell
components/core/tools/scripts/lib_install/ubuntu-jammy/install-all.sh
```

## Build

There are two flavours of the CLP package:

1. `clp-json` for managing JSON logs
2. `clp-text` for managing text logs

:::{note}
Both flavours contain the same binaries but are configured with different values for the
`package.storage_engine` and `package.query_engine` key.
:::

To build the package, run:

```shell
task
```

The build will be in `build/clp-package` and defaults to using the storage and query engines for
`clp-json`.

:::{note}
The `task` command runs `task package` under the hood. In addition to the build, a Docker image
named `clp-package:dev-<user>-<unique-id>` will also be created.
:::

:::{note}
The package includes a `docker-compose.yaml` file that can be used to deploy CLP using [Docker
Compose][docker-compose]. If you want to manually deploy with Docker Compose instead of using the
package scripts, see the [Deployment orchestration][design-deployment-orchestration] design doc for
more information.
:::

To build a releasable tar of either flavour, run:

```shell
task clp-<flavour>-pkg-tar
```

where `<flavour>` is `json` or `text`.

The tar will be written to `build/clp-<flavour>-<os>-<arch>-v<version>.tar.gz`, with appropriate
values for the fields in angle brackets.

## Cleanup

To clean up all build artifacts, run:

```shell
task clean
```

[Docker]: https://docs.docker.com/engine/install/
[docker-compose]: https://docs.docker.com/compose/
[design-deployment-orchestration]: design-deployment-orchestration.md
[Task]: https://taskfile.dev/
[uv]: https://docs.astral.sh/uv/
[y-scope/clp#1352]: https://github.com/y-scope/clp/issues/1352
