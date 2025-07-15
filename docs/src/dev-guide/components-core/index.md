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

To get all dependencies required by CLP core, run the `deps:core` task:

```shell
task deps:core
```

The task will download, build, and install (within the build directory) the following libraries:

| Library                                                               | Version/commit |
|-----------------------------------------------------------------------|----------------|
| [abseil-cpp](https://github.com/abseil/abseil-cpp)                    | 20250512.0     |
| [ANTLR](https://www.antlr.org)                                        | v4.13.2        |
| [Catch2](https://github.com/catchorg/Catch2.git)                      | v2.13.7        |
| [date](https://github.com/HowardHinnant/date.git)                     | v3.0.1         |
| [fmt](https://github.com/fmtlib/fmt)                                  | v10.2.1        |
| [json](https://github.com/nlohmann/json.git)                          | v3.11.3        |
| [log-surgeon](https://github.com/y-scope/log-surgeon)                 | f801a3f        |
| [mongo-cxx-driver](https://github.com/mongodb/mongo-cxx-driver)       | r3.10.2        |
| [simdjson](https://github.com/simdjson/simdjson)                      | v3.13.0        |
| [spdlog](https://github.com/gabime/spdlog)                            | v1.14.1        |
| [SQLite3](https://www.sqlite.org/download.html)                       | v3.36.0        |
| [uftcpp](https://github.com/nemtrif/utfcpp.git)                       | v4.0.6         |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp.git)                    | v0.7.0         |
| [yscope-log-viewer](https://github.com/y-scope/yscope-log-viewer.git) | 969ff35        |
| [ystdlib-cpp](https://github.com/y-scope/ystdlib-cpp.git)             | d80cf86        |

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
