#!/usr/bin/env bash

# Exit on any error, use of undefined variables, or failure within a pipeline
set -euo pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Get the installed go-task version string
task_version=$(task --silent --taskfile "${script_dir}/print-go-task-version.yaml" version)
IFS=. read -r task_major_version task_minor_version _ <<< "${task_version}"

# Check version constraints
if [ "${task_major_version}" -ne "3" ] || \
   [ "${task_minor_version}" -lt "40" ] || \
   [ "${task_minor_version}" -ge "43" ]; then
  echo "Error: Task version ${task_version} is currently unsupported (must be 3.40 <= ver < 3.43)."
  exit 1
fi
