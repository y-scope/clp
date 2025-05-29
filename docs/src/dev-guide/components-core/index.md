# Core

CLP core is the low-level component that performs compression, decompression, and search.

## Requirements

* We have built and tested CLP on the OSes listed [below](#native-environment).
  * If you have trouble building for another OS, file an issue, and we may be able to help.
* A recent compiler that fully supports C++20 features such as
  * std::span
  * std::source_location
* [Task] >= 3.38.0 and < 3.43.0
  * We constrain the version due to unresolved [issues][clp-issue-872].

To build, we require some source dependencies, packages from package managers, and libraries built
from source.

### Set up

To initialize the project, run:

```shell
tools/scripts/deps-download/init.sh
```

### Source Dependencies

To download and build all dependencies required by the core, run this `task` command:

```shell
task deps:core
```

The command above will fetch, build, and install the following exact versions:

| Library            | Version/commit | Notes                               |
|--------------------|----------------|-------------------------------------|
| absl               | 20230802.1     | renamed from abseil-cpp             |
| antlr4             | v4.13.1        | JAR only                            |
| catch2             | v2.13.7        |                                     |
| date               | v3.0.1         |                                     |
| fmt                | v8.0.1         |                                     |
| nlohmann_json      | v3.11.3        | renamed from json                   |
| log-surgeon        | f801a3f        |                                     |
| outcome            | v2.2.9         |                                     |
| simdjson           | v3.6.3         |                                     |
| spdlog             | v1.9.2         |                                     |
| sqlite3            | v3.36.0        |                                     |
| utfcpp             | v4.0.6         |                                     |
| yaml-cpp           | v0.7.0         | target name changed to yaml-cpp     |
| yscope-log-viewer  | 969ff35        |                                     |
| ystdlib            | 2ac1757        | renamed from ystdlib-cpp            |

### Environment

A handful of packages and libraries are required to build CLP. There are two options to use them:

* Install them on your machine and build CLP natively
* Build CLP within a prebuilt docker container that contains the libraries;
  However, this won't work if you need additional libraries that aren't already in the container.

#### Native Environment

See the relevant README for your OS:

* [CentOS Stream 9](centos-stream-9-deps-install)
* [macOS](macos-deps-install)
* [Ubuntu 22.04](ubuntu-jammy-deps-install)

Want to build natively on an OS not listed here? You can file a [feature request][feature-req].

#### Docker Environment

You can use these commands to start a container in which you can build and run CLP:

```shell
# Make sure to change /path/to/clp/components/core and /path/to/my/logs below
docker run --rm -it \
  --name 'clp-build-env' \
  -u$(id -u):$(id -g) \
  -v$(readlink -f /path/to/clp/components/core):/mnt/clp \
  -v$(readlink -f /path/to/my/logs):/mnt/logs \
  ghcr.io/y-scope/clp/clp-core-dependencies-x86-ubuntu-jammy:main \
  /bin/bash -l

cd /mnt/clp
```

Make sure to change `/path/to/clp/components/core` and `/path/to/my/logs` to
the relevant paths on your machine.

## Build

* Configure the cmake project:
  ```shell
  mkdir build
  cd build
  cmake ../
  ```

* Build:
  ```shell
  make -j
  ```

:::{toctree}
:hidden:

centos-stream-9-deps-install
macos-deps-install
ubuntu-jammy-deps-install
regex-utils
:::

[clp-issue-872]: https://github.com/y-scope/clp/issues/872
[feature-req]: https://github.com/y-scope/clp/issues/new?assignees=&labels=enhancement&template=feature-request.yml
[Task]: https://taskfile.dev/
