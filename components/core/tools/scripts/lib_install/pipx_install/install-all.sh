#!/usr/bin/env bash

# Exit on any error, use of undefined variables, or failure within a pipeline
set -euo pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

"${script_dir}/install-cmake.sh"
"${script_dir}/install-go-task.sh"
"${script_dir}/install-uv.sh"
