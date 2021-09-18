CLP is a tool capable of losslessly compressing text logs and searching the compressed logs without decompression.
To learn more about it, you can read our [paper](https://www.usenix.org/system/files/osdi21-rodrigues.pdf).

# Contents
* [Getting Started](#getting-started)
* [Requirements](#requirements)
* [Building](#building)
  * [Source Dependencies](#source-dependencies)
  * [Packages](#packages)
  * [Libraries](#libraries)
  * [Build](#build)
* [Running](#running)
  * [`clp`](#clp)
  * [`clg`](#clg)
* [Next Steps](#next-steps)
  

# Getting Started
CLP is currently released as source, so you'll need to build it before running it.

# Requirements
* We have built and tested CLP on **Ubuntu 18.04 (bionic)** and **Ubuntu 20.04 (focal)**.
  * If you have trouble building for another OS, file an issue and we may be able to help.
* A compiler that supports c++14
* By default, CLP is built with static linking, so you shouldn't need to install any other 
  dependencies to run it.

# Building
* To build, we require some source dependencies, packages from package managers, and libraries built from source.

## Source Dependencies
We use both git submodules and third-party source packages. To download all, you can run this script:
```shell
tools/scripts/deps-download/download-all.sh
```

This will download:
* [Catch2](https://github.com/catchorg/Catch2.git) (v2.13.6)
* [date](https://github.com/HowardHinnant/date.git) (v3.0.1)
* [SQLite3](https://www.sqlite.org/download.html) (v3.36.0)

## Packages
If you're using apt-get, you can use the following command to install all:
```shell
sudo apt-get install -y ca-certificates checkinstall cmake build-essential \
libboost-filesystem-dev libboost-iostreams-dev libboost-program-options-dev \
libssl-dev pkg-config wget zlib1g-dev
```

This will download:
* ca-certificates
* checkinstall
* cmake
* build-essential
* libboost-filesystem-dev
* libboost-iostreams-dev
* libboost-program-options-dev
* libssl-dev
* pkg-config
* wget
* zlib1g-dev

## Libraries
The latest versions of some packages are not offered by apt repositories,
so we've included some scripts to download, compile, and install them:
```shell
./tools/scripts/lib_install/fmtlib.sh 8.0.1
./tools/scripts/lib_install/libarchive.sh 3.5.1
./tools/scripts/lib_install/lz4.sh 1.8.2
./tools/scripts/lib_install/spdlog.sh 1.9.2
./tools/scripts/lib_install/zstandard.sh 1.4.9
```

## Build
* Configure the cmake project:
  ```shell
  mkdir build
  cd build
  cmake ../
  ```

* Build:
  ```shell
  make
  ```

# Running
* CLP contains two executables: `clp` and `clg`
    * `clp` is used for compressing and extracting logs
    * `clg` is used for performing wildcard searches on the compressed logs

## `clp`
To compress some logs:
```shell
./clp c archives-dir /home/my/logs
```
* `archives-dir` is where compressed logs should be output
  * `clp` will create a number of files and directories within, so it's best if this directory is empty
  * You can use the same directory repeatedly and `clp` will add to the compressed logs within.
* `/home/my/logs` is any log file or directory containing log files

To decompress those logs:
```shell
./clp x archive-dir decompressed
```
* `archives-dir` is where the compressed logs were previously stored
* `decompressed` is a directory where they will be decompressed to

You can also decompress a specific file:
```shell
./clp x archive-dir decompressed /my/file/path.log
```
* `/my/file/path.log` is the uncompressed file's path (the one that was passed to `clp` for compression) 

More usage instructions can be found by running:
```shell
./clp --help
```

## `clg`
To search the compressed logs:
```shell
./clg archives-dir " a *wildcard* search phrase "
```
* `archives-dir` is where the compressed logs were previously stored
* The search phrase can contain the `*` wildcard which matches 0 or more characters, or the `?` wildcard which matches any single character.

Similar to `clp`, `clg` can search a single file:
```shell
./clg archives-dir " a *wildcard* search phrase " /my/file/path.log
```
* `/my/file/path.log` is the uncompressed file's path (the one that was passed to `clp` for compression)

More usage instructions can be found by running:
```shell
./clg --help
```

# Next Steps
This is our initial open-source release which we will be constantly updating with bug fixes, features, etc.
If you would like a feature or want to report a bug, please file an issue and we'll be happy to engage.
We also welcome any contributions!
