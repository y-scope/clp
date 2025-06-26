# Core

CLP core is the low-level component that performs compression, decompression, and search.

## Requirements

* We have built and tested CLP on the OSes listed [below](#native-build-environment).
  * If you have trouble building for another OS, file an issue, and we may be able to help.
* A recent compiler that fully supports C++20 features such as
  * std::span
  * std::source_location
* [Task] >= 3.38.0 and < 3.43.0
  * We constrain the version due to unresolved [issues][clp-issue-872].

To build, we require some source dependencies, packages from package managers, and libraries built
from source.

### Build Environment

A handful of packages and libraries are required to build CLP. There are three options to use them:

* Install them on your machine and build CLP natively
* Build CLP within a prebuilt docker container that contains the libraries;
  However, this won't work if you need additional libraries that aren't already in the container.
* If you just want to quickly build a CLP binary compatible with most platforms, you can run the following script:
  ```shell
  # Build CLP binary using manylinux_2_28 for x86_64 or aarch64 platforms
  /path/to/clp/components/core/tools/scripts/utils/build-with-docker.py \
  --output <output_directory> \
  [--platform linux/amd64|linux/arm64]
  ```
  * `--output <output_directory>`: Directory to place the built CLP binary.
  * `--platform linux/amd64|linux/arm64` (optional): Target platform. Defaults to linux/amd64 if not specified.


#### Native Build Environment

See the relevant README for your OS:

* [manylinux_2_28](manylinux_2_28-deps-install)
* [CentOS Stream 9](centos-stream-9-deps-install)
* [macOS](macos-deps-install)
* [Ubuntu 22.04](ubuntu-jammy-deps-install)

Want to build natively on an OS not listed here? You can file a [feature request][feature-req].

#### Docker Build Environments

You can use these commands to start a container in which you can manually build and run CLP:

* Start a build container

  ```shell
  # Make sure to change /path/to/clp/ and /path/to/my/logs below
  docker run --rm -it \
    --name 'clp-build-env' \
    -u$(id -u):$(id -g) \
    -v$(readlink -f /path/to/clp):/mnt/clp \
    -v$(readlink -f /path/to/my/logs):/mnt/logs \
    ghcr.io/y-scope/clp/clp-core-dependencies-x86-ubuntu-jammy:main \
    /bin/bash -l
  
  cd /mnt/clp
  ```

### Set up

After the build environment is initialized, we begin by initializing the project:

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
| [fmt](https://github.com/fmtlib/fmt)                                  | v8.0.1         |
| [json](https://github.com/nlohmann/json.git)                          | v3.11.3        |
| [log-surgeon](https://github.com/y-scope/log-surgeon)                 | f801a3f        |
| [mongo-cxx-driver](https://github.com/mongodb/mongo-cxx-driver)       | r3.10.2        |
| [simdjson](https://github.com/simdjson/simdjson)                      | v3.13.0        |
| [spdlog](https://github.com/gabime/spdlog)                            | v1.9.2         |
| [SQLite3](https://www.sqlite.org/download.html)                       | v3.36.0        |
| [uftcpp](https://github.com/nemtrif/utfcpp.git)                       | v4.0.6         |
| [yaml-cpp](https://github.com/jbeder/yaml-cpp.git)                    | v0.7.0         |
| [yscope-log-viewer](https://github.com/y-scope/yscope-log-viewer.git) | 969ff35        |
| [ystdlib-cpp](https://github.com/y-scope/ystdlib-cpp.git)             | d80cf86        |


### Build

* Configure the cmake project:

  ```shell
  cd components/core
  mkdir build
  cd build
  cmake ../
  ```

* Build:

  ```shell
  make -j
  ```

> Make sure to change `/path/to/clp/` and `/path/to/my/logs` to the relevant paths on your machine.

:::{toctree}
:hidden:
manylinux_2_28-deps-install
centos-stream-9-deps-install
macos-deps-install
ubuntu-jammy-deps-install
regex-utils
:::

[clp-issue-872]: https://github.com/y-scope/clp/issues/872
[feature-req]: https://github.com/y-scope/clp/issues/new?assignees=&labels=enhancement&template=feature-request.yml
[Task]: https://taskfile.dev/
