# Building a package

This page describes the requirements and steps to build a CLP package. If you're looking for a
prebuilt version instead, check out the [releases](https://github.com/y-scope/clp/releases) page.

## Requirements

* An x86_64 Ubuntu 20.04 (Focal) machine or container
  * At runtime, the CLP package uses an Ubuntu Focal container, so we need to build in a matching
    environment.
  * It should be possible to build a package for a different environment, it just requires a some
    extra configuration.
* Python 3.8 or newer
* python3-venv
* [Node.js 14](https://nodejs.org/download/release/v14.21.3/) (Meteor.js only
  [supports](https://docs.meteor.com/install#prereqs-node) Node.js versions >= 10 and <= 14)
* [Meteor.js](https://docs.meteor.com/install.html#installation)
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

To build both flavours, run:

```shell
task
```

The builds will be written to `build/clp-json-package` and `build/clp-text-package`, respectively.

To build a single flavour, run:

```shell
task clp-<flavour>-pkg
```

where `<flavour>` is `json` or `text`.

To build a releasable tar of either package, run:

```shell
task clp-<flavour>-pkg-tar
```

where `<flavour>` is `json` or `text`.

## Cleanup

To clean up all build artifacts, run:

```shell
task clean
```
