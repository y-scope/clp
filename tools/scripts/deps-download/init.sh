#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
project_root_dir="$script_dir/../../../"
download_dep_script="$script_dir/download-dep.py"

python3 "${download_dep_script}" \
    https://github.com/y-scope/yscope-dev-utils/archive/2caa3dcf.zip \
    yscope-dev-utils-2caa3dcfbbccff052d179e643a509d8ad05bc217 \
    "${project_root_dir}/tools/yscope-dev-utils" \
    --extract
