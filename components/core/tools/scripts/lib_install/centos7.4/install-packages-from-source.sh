#!/usr/bin/env bash

# Exit on any error
set -e

# Enable gcc 10
source /opt/rh/devtoolset-10/enable

# Enable git
source /opt/rh/rh-git227/enable

# Error on undefined variable
# NOTE: We enable this *after* sourcing the scripts above since we can't guarantee they won't have
# unbound variables in them.
set -u

# NOTE: cmake and boost must be installed first since the remaining packages depend on them
./tools/scripts/lib_install/install-cmake.sh 3.21.2
./tools/scripts/lib_install/install-boost.sh 1.76.0

./tools/scripts/lib_install/fmtlib.sh 8.0.1
./tools/scripts/lib_install/libarchive.sh 3.5.1
./tools/scripts/lib_install/lz4.sh 1.8.2
./tools/scripts/lib_install/mariadb-connector-c.sh 3.2.3
./tools/scripts/lib_install/mongoc.sh 1.24.4
./tools/scripts/lib_install/mongocxx.sh 3.8.0
./tools/scripts/lib_install/msgpack.sh 6.0.0
./tools/scripts/lib_install/spdlog.sh 1.9.2
./tools/scripts/lib_install/zstandard.sh 1.4.9
