#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
project_root_dir="$script_dir/../../../"
download_dep_script="$script_dir/download-dep.py"

python3 "${download_dep_script}" \
    https://github.com/y-scope/yscope-dev-utils/archive/e300d1b.zip \
    yscope-dev-utils-e300d1bab4c2f33cbf9ddf9f0c08185faf035070 \
    "${project_root_dir}/tools/yscope-dev-utils" \
    --extract
