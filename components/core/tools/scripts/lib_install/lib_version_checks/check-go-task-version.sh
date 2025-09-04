#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Get the installed go-task version string
task_version=$(task --silent --taskfile "${script_dir}/print-go-task-version.yaml" version)
IFS=. read -r task_major_version task_minor_version task_patch_version <<< "${task_version}"

# Check version constraints
    # We lock `task` to version 3.44.0 to avoid https://github.com/y-scope/clp-ffi-js/issues/110
if [ "${task_major_version}" -ne "3" ] || \
   [ "${task_minor_version}" -ne "44" ] || \
   [ "${task_patch_version}" -ne "0" ]; then
  echo "Error: Task version ${task_version} is currently unsupported (must be 3.44.0)."
  exit 1
fi
