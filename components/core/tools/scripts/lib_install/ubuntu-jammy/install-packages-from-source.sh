#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
lib_install_scripts_dir=$script_dir/..

"$lib_install_scripts_dir"/fmtlib.sh 8.0.1
"$lib_install_scripts_dir"/libarchive.sh 3.5.1
"$lib_install_scripts_dir"/lz4.sh 1.8.2
"$lib_install_scripts_dir"/mongoc.sh 1.24.4
"$lib_install_scripts_dir"/mongocxx.sh 3.8.0
"$lib_install_scripts_dir"/msgpack.sh 6.0.0
"$lib_install_scripts_dir"/spdlog.sh 1.9.2
"$lib_install_scripts_dir"/zstandard.sh 1.4.9
