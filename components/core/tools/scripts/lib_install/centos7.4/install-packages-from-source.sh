#!/usr/bin/env bash

# Enable gcc 10
source /opt/rh/devtoolset-10/enable

# NOTE: cmake and boost must be installed first since the remaining packages depend on them
./tools/scripts/lib_install/install-cmake.sh 3.21.2
./tools/scripts/lib_install/install-boost.sh 1.76.0

./tools/scripts/lib_install/fmtlib.sh 8.0.1
./tools/scripts/lib_install/libarchive.sh 3.5.1
./tools/scripts/lib_install/lz4.sh 1.8.2
./tools/scripts/lib_install/mariadb-connector-c.sh 3.2.3
./tools/scripts/lib_install/msgpack.sh 6.0.0
./tools/scripts/lib_install/spdlog.sh 1.9.2
./tools/scripts/lib_install/zstandard.sh 1.4.9
