#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

./tools/scripts/lib_install/fmtlib.sh 8.0.1
./tools/scripts/lib_install/libarchive.sh 3.5.1
./tools/scripts/lib_install/lz4.sh 1.8.2
./tools/scripts/lib_install/mongoc.sh 1.24.4
./tools/scripts/lib_install/mongocxx.sh 3.8.0
./tools/scripts/lib_install/msgpack.sh 6.0.0
./tools/scripts/lib_install/spdlog.sh 1.9.2
./tools/scripts/lib_install/zstandard.sh 1.4.9
