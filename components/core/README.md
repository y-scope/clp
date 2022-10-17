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
  * [`clp`](#clp)
  * [`clg`](#clg)
  * [`make-dictionaries-readable`](#make-dictionaries-readable)
* [Parallel Compression](#parallel-compression)

## Requirements

* We have built and tested CLP on **Ubuntu 18.04 (bionic)** and **Ubuntu 20.04 (focal)**.
  * If you have trouble building for another OS, file an issue, and we may be able to help.
* A compiler that supports C++17 (e.g., gcc-8)

## Building

* To build, we require some source dependencies, packages from package managers, and libraries built from source.

### Source Dependencies

We use both git submodules and third-party source packages. To download all, you can run this script:
```shell
tools/scripts/deps-download/download-all.sh
```

This will download:
* [Catch2](https://github.com/catchorg/Catch2.git) (v2.13.6)
* [date](https://github.com/HowardHinnant/date.git) (v3.0.1)
* [json](https://github.com/nlohmann/json.git) (v3.10.2)
* [SQLite3](https://www.sqlite.org/download.html) (v3.36.0)
* [yaml-cpp](https://github.com/jbeder/yaml-cpp.git) (v0.7.0)

### Environment

A handful of packages and libraries are required to build CLP. There are two options to use them:

* Install them on your machine and build CLP natively
* Build CLP within a prebuilt docker container that contains the libraries;
  However, this won't work if you need additional libraries that aren't already in the container.

#### Native Environment

*Packages*

If you're using apt-get, you can use the following command to install all:
```shell
sudo apt-get install -y ca-certificates checkinstall cmake curl build-essential \
libboost-filesystem-dev libboost-iostreams-dev libboost-program-options-dev \
libssl-dev pkg-config rsync zlib1g-dev
```

This will download:
* ca-certificates
* checkinstall
* cmake
* curl
* build-essential
* libboost-filesystem-dev
* libboost-iostreams-dev
* libboost-program-options-dev
* libssl-dev
* pkg-config
* rsync
* zlib1g-dev

*Libraries*

The latest versions of some packages are not offered by apt repositories,
so we've included some scripts to download, compile, and install them:
```shell
./tools/scripts/lib_install/fmtlib.sh 8.0.1
./tools/scripts/lib_install/libarchive.sh 3.5.1
./tools/scripts/lib_install/lz4.sh 1.8.2
./tools/scripts/lib_install/mariadb-connector-c.sh 3.2.3
./tools/scripts/lib_install/msgpack.sh 4.1.1
./tools/scripts/lib_install/spdlog.sh 1.9.2
./tools/scripts/lib_install/zstandard.sh 1.4.9
```

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
  /bin/bash

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
  make
  ```

## Running

* CLP contains two executables: `clp` and `clg`
    * `clp` is used for compressing and extracting logs
    * `clg` is used for performing wildcard searches on the compressed logs

### `clp`

To compress some logs without a schema file:
```shell
./clp c archives-dir /home/my/logs
```
* `archives-dir` is where compressed logs should be output
  * `clp` will create a number of files and directories within, so it's best if this directory is empty
  * You can use the same directory repeatedly and `clp` will add to the compressed logs within.
* `/home/my/logs` is any log file or directory containing log files
* In this mode, `clp` will use heuristics to determine what are the variables in
  each uncompressed message.
  * The heuristics roughly correspond to the example schema file in
    `config/schemas.txt`.

To compress with a user-defined schema file:
```shell
./clp c --schema-path path-to-schema-file archives-dir /home/my/logs 
```
* `path-to-schema-file` is the location of a schema file. For more details on 
  schema files, see README-Schema.md.

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

### `clg`

To search the compressed logs:
```shell
./clg archives-dir " a *wildcard* search phrase "
```
* `archives-dir` is where the compressed logs were previously stored
* For archives compressed without a schema file:
  * The search phrase can contain the `*` wildcard which matches 0 or more
    characters, or the `?` wildcard which matches any single character.
* For archives compressed using a schema file:
  * `*` may only represent non-delimiter characters.

Similar to `clp`, `clg` can search a single file:
```shell
./clg archives-dir " a *wildcard* search phrase " /my/file/path.log
```
* `/my/file/path.log` is the uncompressed file's path (the one that was passed to `clp` for compression)

More usage instructions can be found by running:
```shell
./clg --help
```

### `make-dictionaries-readable`

If you'd like to convert the dictionaries of an individual archive into a human-readable form, you 
can use `make-dictionaries-readable`.

```shell
./make-dictionaries-readable archive-path <output dir>
```
* `archive-path` is a path to a specific archive (inside `archives-dir`)

See the `make-dictionaries-readable` [README](src/utils/make_dictionaries_readable/README.md) for 
details on the output format. 

## Parallel Compression

By default, `clp` uses an embedded SQLite database, so each directory containing archives can only
be accessed by a single `clp` instance.

To enable parallel compression to the same archives directory, `clp`/`clg` can be configured to
use a MySQL-type database (MariaDB) as follows: 

* Install and configure MariaDB using the instructions for your platform
* Create a user that has privileges to create databases, create tables, insert records, and delete
  records.
* Copy and change `config/metadata-db.yml`, setting the type to `mysql` and uncommenting the MySQL 
  parameters.
* Install the MariaDB and PyYAML Python packages `pip3 install mariadb PyYAML`
  * This is necessary to run the database initialization script. If you prefer, you can run the 
    SQL statements in `tools/scripts/db/init-db.py` directly.
* Run `tools/scripts/db/init-db.py` with the updated config file. This will initialize the 
  database CLP requires.
* Run `clp` or `clg` as before, with the addition of the `--db-config-file` option pointing at 
  the updated config file.
* To compress in parallel, simply run another instance of `clp` concurrently.

Note that currently, decompression (`clp x`) and search (`clg`) can only be run with a single 
instance. We are in the process of open-sourcing parallelized versions of these as well.
