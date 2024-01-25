# CLP Core

CLP core is the low-level component that performs compression, decompression, and search.

## Contents

* [Requirements](#requirements)
* [Building](#building)
  * [Source Dependencies](#source-dependencies)
  * [Environment](#environment)
    * [Native Environment](#native-environment)
    * [Docker Environment](#docker-environment)
  * [Build](#build)
* [Running](#running)


## Requirements

* We have built and tested CLP on the OSes listed 
  [below](https://github.com/y-scope/clp/tree/main/components/core#native-environment).
  * If you have trouble building for another OS, file an issue, and we may be able to help.
* A compiler that supports C++17 and std::span (e.g., gcc-10)

## Building

* To build, we require some source dependencies, packages from package managers, and libraries built from source.

### Source Dependencies

We use both git submodules and third-party source packages. To download all, you can run this script:
```shell
tools/scripts/deps-download/download-all.sh
```

This will download:
* [abseil-cpp](https://github.com/abseil/abseil-cpp) (20230802.1)
* [ANTLR](https://www.antlr.org) (v4.13.1)
* [Catch2](https://github.com/catchorg/Catch2.git) (v2.13.7)
* [date](https://github.com/HowardHinnant/date.git) (v3.0.1)
* [json](https://github.com/nlohmann/json.git) (v3.10.4)
* [log-surgeon](https://github.com/y-scope/log-surgeon) (895f464)
* [simdjson](https://github.com/simdjson/simdjson) (v3.6.3)
* [SQLite3](https://www.sqlite.org/download.html) (v3.36.0)
* [yaml-cpp](https://github.com/jbeder/yaml-cpp.git) (v0.7.0)

### Environment

A handful of packages and libraries are required to build CLP. There are two options to use them:

* Install them on your machine and build CLP natively
* Build CLP within a prebuilt docker container that contains the libraries;
  However, this won't work if you need additional libraries that aren't already in the container.

#### Native Environment

See the relevant README for your OS:

* [CentOS 7.4](./tools/scripts/lib_install/centos7.4/README.md)
* [macOS 12](./tools/scripts/lib_install/macos-12/README.md)
* [Ubuntu 20.04](./tools/scripts/lib_install/ubuntu-focal/README.md)
* [Ubuntu 22.04](./tools/scripts/lib_install/ubuntu-jammy/README.md)

Want to build natively on an OS not listed here? You can file a [feature request](https://github.com/y-scope/clp/issues/new?assignees=&labels=enhancement&template=feature-request.yml).

#### Docker Environment

You can use these commands to start a container in which you can build and run CLP:

```shell
# Make sure to change /path/to/clp/components/core and /path/to/my/logs below
docker run --rm -it \
  --name 'clp-build-env' \
  -u$(id -u):$(id -g) \
  -v$(readlink -f /path/to/clp/components/core):/mnt/clp \
  -v$(readlink -f /path/to/my/logs):/mnt/logs \
  ghcr.io/y-scope/clp/clp-core-dependencies-x86-ubuntu-focal:main \
  /bin/bash -l

cd /mnt/clp
```

Make sure to change `/path/to/clp/components/core` and `/path/to/my/logs` to
the relevant paths on your machine.

### Build

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

## Running

* CLP contains four core executables: `clp`, `clg`, `clp-s` and `glt`.
  * `clp` is used for compressing and extracting unstructured (plain text) logs.
  * `clg` is used for performing wildcard searches on the compressed unstructured logs.
  * `clp-s` is used for compressing and searching semi-structured logs (e.g., JSON) with support for
    handling highly dynamic schemas.
  * `glt` is a version of clp specialized for searching unstructured (plain text) logs.

See [Using CLP for unstructured logs](../../docs/core/clp-unstructured.md),
 [Using CLP for semi-structured logs](../../docs/core/clp-structured.md) and
 [Using GLT for unstructured logs](../../docs/core/glt.md) for usage instructions.
