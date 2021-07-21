#!/bin/bash

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
project_root_dir=${script_dir}/../../../

cd ${project_root_dir}
git submodule update --init --recursive
python3 ${script_dir}/download-dep.py ${script_dir}/sqlite3.json
