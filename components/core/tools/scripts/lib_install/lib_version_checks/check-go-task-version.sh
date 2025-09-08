#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

task_version=$(task --silent --taskfile "${script_dir}/print-go-task-version.yaml")

# We lock to version 3.44.0 to avoid https://github.com/y-scope/clp-ffi-js/issues/110
if [[ "${task_version}" != "3.44.0" ]]; then
  echo "Error: Task version ${task_version} is currently unsupported (must be 3.44.0)."
  exit 1
fi
