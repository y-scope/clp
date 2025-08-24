#!/usr/bin/env bash

# Exit on any error, use of undefined variables, or failure within a pipeline
set -euo pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# TODO: https://github.com/y-scope/clp/issues/795
"${script_dir}/../check-cmake-version.sh"

# TODO: https://github.com/y-scope/clp/issues/872
"${script_dir}/../check-go-task-version.sh"
