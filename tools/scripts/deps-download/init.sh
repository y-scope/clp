#!/bin/bash

# Stop on error
set -e

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
project_root_dir="$script_dir"/../../../
download_dep_script="$script_dir"/download-dep.py

python3 "${download_dep_script}" \
  https://github.com/y-scope/yscope-dev-utils/archive/ff1611e6.zip \
  yscope-dev-utils-ff1611e6f9b116da27dc7f8f71797829c22d0b1a \
  "${project_root_dir}/tools/yscope-dev-utils" \
  --extract
