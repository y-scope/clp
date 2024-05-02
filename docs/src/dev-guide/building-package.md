# Building the package

This page describes the requirements and steps to build the CLP package. If you're looking for a
prebuilt version instead, check out the [releases](https://github.com/y-scope/clp/releases) page.

## Requirements

* An x86_64 Ubuntu 20.04 (Focal) machine or container
  * At runtime, the CLP package uses an Ubuntu Focal container, so we need to build in a matching
    environment.
  * It should be possible to build a package for a different environment, it just requires a some
    extra configuration.
* Python 3.8 or newer
* python3-venv
* [Task](https://taskfile.dev/)

## Setup

Download CLP core's source dependencies:

```shell
components/core/tools/scripts/deps-download/download-all.sh
```

Install CLP core's dependencies

```shell
components/core/tools/ubuntu-focal/install-all.sh
```

## Build

There are two flavours of the CLP package:

1. `clp-json` for managing JSON logs
2. `clp-text` for managing text logs

:::{note}
Both flavours contain the same binaries but are configured with different values for the
`package.storage_engine` key.
:::

To build the package, run:

```shell
task package
```

The build will be in `build/clp-package` and defaults to using the storage engine for `clp-text`.

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
