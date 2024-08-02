#!/bin/bash

# Stop on error
set -e

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
project_root_dir="$script_dir"/../../../
component_root_dir="${project_root_dir}/component"
shared_script_dir="${project_root_dir}/tools/scripts"
download_dep_script="${shared_script_dir}/download-dep.py"

cd "${component_root_dir}"

if [ -e "$project_root_dir/.git" ] ; then
  git submodule update --init --recursive
else
  python3 "${download_dep_script}" "${script_dir}/yscope-log-viewer.json" "${component_root_dir}"
fi
