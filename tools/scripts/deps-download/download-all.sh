#!/bin/bash

# Stop on error
set -e

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
project_root_dir="$script_dir"/../../../
download_dep_script="${project_root_dir}"/tools/scripts/download-dep.py

if [ -e "$project_root_dir/.git" ] ; then
  git submodule update --init --recursive
else
  python3 "${download_dep_script}" "${script_dir}/yscope-log-viewer.json" "${project_root_dir}"
  python3 "${download_dep_script}" "${script_dir}/yscope-dev-utils.json" "${project_root_dir}"
fi