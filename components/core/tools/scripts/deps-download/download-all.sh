#!/bin/bash

# Stop on error
set -e

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
project_root_dir="$script_dir"/../../../../../
core_root_dir="${project_root_dir}"/components/core
download_dep_script="${project_root_dir}"/tools/scripts/download-dep.py

mkdir -p "${component_root_dir}"/submodules

# We don't use a git submodule for sqlite3 since that would require building the
# sqlite amalgamation
python3 "${download_dep_script}" "${script_dir}/sqlite3.json" "${core_root_dir}"
python3 "${download_dep_script}" "${script_dir}/antlr4.json" "${core_root_dir}"

if [ -e "$project_root_dir/.git" ] ; then
  git submodule update --init --recursive
else
  python3 "${download_dep_script}" "${script_dir}/abseil-cpp.json" "${core_root_dir}"
  python3 "${download_dep_script}" "${script_dir}/Catch2.json" "${core_root_dir}"
  python3 "${download_dep_script}" "${script_dir}/date.json" "${core_root_dir}"
  python3 "${download_dep_script}" "${script_dir}/json.json" "${core_root_dir}"
  python3 "${download_dep_script}" "${script_dir}/log-surgeon.json" "${core_root_dir}"
  python3 "${download_dep_script}" "${script_dir}/outcome.json" "${core_root_dir}"
  python3 "${download_dep_script}" "${script_dir}/simdjson.json" "${core_root_dir}"
  python3 "${download_dep_script}" "${script_dir}/yaml-cpp.json" "${core_root_dir}"
fi
