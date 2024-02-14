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
lib_install_scripts_dir=$script_dir/..

# NOTE: cmake and boost must be installed first since the remaining packages depend on them
"$lib_install_scripts_dir"/install-cmake.sh 3.21.2
"$lib_install_scripts_dir"/install-boost.sh 1.76.0

"$lib_install_scripts_dir"/fmtlib.sh 8.0.1
"$lib_install_scripts_dir"/libarchive.sh 3.5.1
"$lib_install_scripts_dir"/lz4.sh 1.8.2
"$lib_install_scripts_dir"/mariadb-connector-c.sh 3.2.3
"$lib_install_scripts_dir"/mongoc.sh 1.24.4
"$lib_install_scripts_dir"/mongocxx.sh 3.8.0
"$lib_install_scripts_dir"/msgpack.sh 6.0.0
"$lib_install_scripts_dir"/spdlog.sh 1.9.2
"$lib_install_scripts_dir"/zstandard.sh 1.4.9
