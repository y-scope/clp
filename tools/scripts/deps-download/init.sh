#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
project_root_dir="$script_dir/../../../"
download_dep_script="$script_dir/download-dep.py"

python3 "${download_dep_script}" \
    https://github.com/y-scope/yscope-dev-utils/archive/2693d8b.zip \
    yscope-dev-utils-2693d8b69050b8d48f96ae60cfce7be8626d3875 \
    "${project_root_dir}/tools/yscope-dev-utils" \
    --extract
