# Core

CLP core is the low-level component that performs compression, decompression, and search.

## Requirements

* We have built and tested CLP on the OSes listed [below](#native-environment).
  * If you have trouble building for another OS, file an issue, and we may be able to help.
* A recent compiler that fully supports C++20 features such as
  * std::span
* [CMake] >= 3.23.0 and < 4.0.0
  * Minimum version 3.23.0 is required for [yscope-dev-utils].
  * We constrain the version to < 4.0.0 due to [y-scope/clp#795].
* [Task] 3.44.0
  * We pin the version to 3.44.0 due to [y-scope/clp#1352].
* [uv] >= 0.8

To build, we require some source dependencies, packages from package managers, and libraries built
from source.

### Set up

To initialize the project, run:

```shell
tools/scripts/deps-download/init.sh
```

### Source Dependencies

To get all dependencies required by CLP core, run the `deps:core` task:

```shell
task deps:core
```

The task will download, build, and install (within the build directory) the following libraries:

| Library                                                               | Version/commit |
|-----------------------------------------------------------------------|----------------|
| [abseil-cpp](https://github.com/abseil/abseil-cpp)                    | 20250512.0     |
| [ANTLR](https://www.antlr.org)                                        | v4.13.2        |
| [Boost](https://github.com/boostorg/boost)                            | v1.87.0        |
| [Catch2](https://github.com/catchorg/Catch2)                          | v3.8.0         |
| [date](https://github.com/HowardHinnant/date)                         | v3.0.1         |
| [fmt](https://github.com/fmtlib/fmt)                                  | v11.2.0        |
| [liblzma](https://github.com/tukaani-project/xz)                      | v5.8.1         |
| [log-surgeon](https://github.com/y-scope/log-surgeon)                 | 840f262        |
| [lz4](https://github.com/lz4/lz4)                                     | v1.10.0        |
| [microsoft.gsl](https://github.com/microsoft/GSL)                     | v4.0.0         |
| [mongo-cxx-driver](https://github.com/mongodb/mongo-cxx-driver)       | r4.1.1         |
| [msgpack-cxx](https://github.com/msgpack/msgpack-c/tree/cpp_master)   | v7.0.0         |
| [nlohmann_json](https://github.com/nlohmann/json)                     | v3.11.3        |
| [simdjson](https://github.com/simdjson/simdjson)                      | v3.13.0        |
| [spdlog](https://github.com/gabime/spdlog)                            | v1.15.3        |
| [SQLite3](https://www.sqlite.org/download.html)                       | v3.36.0        |
| [utfcpp](https://github.com/nemtrif/utfcpp)                           | v4.0.6         |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp)                        | v0.7.0         |
| [yscope-log-viewer](https://github.com/y-scope/yscope-log-viewer)     | 3abe4cc        |
| [ystdlib-cpp](https://github.com/y-scope/ystdlib-cpp)                 | 9ed78cd        |
| [zlib](https://github.com/madler/zlib)                                | v1.3.1         |
| [zstd](https://github.com/facebook/zstd)                              | v1.5.7         |

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

## Test

```shell
task tests:integration:core
```

:::{toctree}
:hidden:

centos-stream-9-deps-install
macos-deps-install
ubuntu-jammy-deps-install
regex-utils
:::

[CMake]: https://cmake.org/
[feature-req]: https://github.com/y-scope/clp/issues/new?assignees=&labels=enhancement&template=feature-request.yaml
[Task]: https://taskfile.dev/
[uv]: https://docs.astral.sh/uv/
[y-scope/clp#795]: https://github.com/y-scope/clp/issues/795
[y-scope/clp#1352]: https://github.com/y-scope/clp/issues/1352
[yscope-dev-utils]: https://github.com/y-scope/yscope-dev-utils
