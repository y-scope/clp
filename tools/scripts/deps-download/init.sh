#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
project_root_dir="$script_dir/../../../"
download_dep_script="$script_dir/download-dep.py"

readonly YSCOPE_DEV_UTILS_COMMIT_SHA="73d6e964d1432dd5f5d7a9c3d945437d689834cc"
python3 "${download_dep_script}" \
    "https://github.com/Bill-hbrhbr/yscope-dev-utils/archive/${YSCOPE_DEV_UTILS_COMMIT_SHA}.zip" \
    "yscope-dev-utils-${YSCOPE_DEV_UTILS_COMMIT_SHA}" \
    "${project_root_dir}/tools/yscope-dev-utils" \
    --extract
