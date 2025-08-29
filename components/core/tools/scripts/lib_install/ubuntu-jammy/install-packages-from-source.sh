#!/usr/bin/env bash

# Exit on any error, use of undefined variables, or failure within a pipeline
set -euo pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
lib_install_scripts_dir=$script_dir/..

"$lib_install_scripts_dir"/libarchive.sh 3.5.1
"$lib_install_scripts_dir"/liblzma.sh 5.8.1
"$lib_install_scripts_dir"/lz4.sh 1.10.0
"$lib_install_scripts_dir"/zstandard.sh 1.5.7
