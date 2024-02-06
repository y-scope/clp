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

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
scripts_prefix=$script_dir/..

# NOTE: cmake and boost must be installed first since the remaining packages depend on them
"$scripts_prefix"/install-cmake.sh 3.21.2
"$scripts_prefix"/install-boost.sh 1.76.0

"$scripts_prefix"/fmtlib.sh 8.0.1
"$scripts_prefix"/libarchive.sh 3.5.1
"$scripts_prefix"/lz4.sh 1.8.2
"$scripts_prefix"/mariadb-connector-c.sh 3.2.3
"$scripts_prefix"/mongoc.sh 1.24.4
"$scripts_prefix"/mongocxx.sh 3.8.0
"$scripts_prefix"/msgpack.sh 6.0.0
"$scripts_prefix"/spdlog.sh 1.9.2
"$scripts_prefix"/zstandard.sh 1.4.9
