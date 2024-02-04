# Building

This page describes the requirements and steps to build the CLP package. If you're looking for a
prebuilt version instead, check out the [releases](https://github.com/y-scope/clp/releases) page.

# Requirements

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

# Setup

Download CLP core's source dependencies:

```shell
components/core/tools/scripts/deps-download/download-all.sh
```

Install CLP core's dependencies

```shell
components/core/tools/ubuntu-focal/install-all.sh
```

# Build

To build the package as a tar ball, run;

```shell
task package-tar
```

The built tar will be output in the `build` directory.

# Cleanup

To clean up the package, run:

```shell
task clean-package
```
  
To clean up all build artifacts, run:

```shell
task clean
```
