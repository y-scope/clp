#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
lib_install_scripts_dir=$script_dir/..

# NOTE: boost must be installed first since the remaining packages depend on it
"$lib_install_scripts_dir"/install-boost.sh 1.89.0

"$lib_install_scripts_dir"/libarchive.sh 3.5.1
"$lib_install_scripts_dir"/liblzma.sh 5.8.1
"$lib_install_scripts_dir"/lz4.sh 1.10.0
"$lib_install_scripts_dir"/msgpack.sh 7.0.0
"$lib_install_scripts_dir"/zstandard.sh 1.5.7
