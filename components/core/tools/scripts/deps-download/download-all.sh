#!/bin/bash

# Stop on error
set -e

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
project_root_dir=${script_dir}/../../../../../

cd "${project_root_dir}"
mkdir -p submodules
python3 "${script_dir}/download-dep.py" "${script_dir}/Catch2.json"
python3 "${script_dir}/download-dep.py" "${script_dir}/date.json"
python3 "${script_dir}/download-dep.py" "${script_dir}/json.json"
python3 "${script_dir}/download-dep.py" "${script_dir}/sqlite3.json"
python3 "${script_dir}/download-dep.py" "${script_dir}/yaml-cpp.json"
