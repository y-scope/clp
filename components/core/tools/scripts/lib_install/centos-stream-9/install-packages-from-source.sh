#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
lib_install_scripts_dir="${script_dir}/.."

"${lib_install_scripts_dir}/liblzma.sh" 5.8.1
