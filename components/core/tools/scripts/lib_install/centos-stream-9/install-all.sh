#!/usr/bin/env bash

# Exit on any error, use of undefined variables, or failure within a pipeline
set -euo pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

"${script_dir}/install-prebuilt-packages.sh"
"${script_dir}/install-packages-from-source.sh"

"${script_dir}/../../lib_version_checks/check-build-tool-versions.sh"
