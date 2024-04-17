#!/bin/bash

# Stop on error
set -e

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
project_root_dir="$script_dir"/../../../../../
component_root_dir="$script_dir"/../../../

cd "${component_root_dir}"
mkdir -p submodules

# We don't use a git submodule for sqlite3 since that would require building the
# sqlite amalgamation
python3 "${script_dir}/download-dep.py" "${script_dir}/sqlite3.json"
python3 "${script_dir}/download-dep.py" "${script_dir}/antlr4.json"

if [ -e "$project_root_dir/.git" ] ; then
  git submodule update --init --recursive
else
  python3 "${script_dir}/download-dep.py" "${script_dir}/abseil-cpp.json"
  python3 "${script_dir}/download-dep.py" "${script_dir}/boost-outcome.json"
  python3 "${script_dir}/download-dep.py" "${script_dir}/Catch2.json"
  python3 "${script_dir}/download-dep.py" "${script_dir}/date.json"
  python3 "${script_dir}/download-dep.py" "${script_dir}/json.json"
  python3 "${script_dir}/download-dep.py" "${script_dir}/log-surgeon.json"
  python3 "${script_dir}/download-dep.py" "${script_dir}/simdjson.json"
  python3 "${script_dir}/download-dep.py" "${script_dir}/yaml-cpp.json"
fi
