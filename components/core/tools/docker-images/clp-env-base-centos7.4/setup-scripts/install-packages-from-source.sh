#!/bin/bash

# Enable gcc 7
source /opt/rh/devtoolset-7/enable

# NOTE: cmake and boost must be installed first since the remaining packages depend on them
./tools/docker-images/clp-env-base-centos7.4/setup-scripts/install-cmake.sh 3.21.2
./tools/docker-images/clp-env-base-centos7.4/setup-scripts/install-boost.sh 1.76.0

./tools/scripts/lib_install/fmtlib.sh 8.0.1
./tools/scripts/lib_install/libarchive.sh 3.5.1
./tools/scripts/lib_install/lz4.sh 1.8.2
./tools/scripts/lib_install/mariadb-connector-c.sh 3.2.3
./tools/scripts/lib_install/msgpack.sh 4.1.1
./tools/scripts/lib_install/spdlog.sh 1.9.2
./tools/scripts/lib_install/zstandard.sh 1.4.9
