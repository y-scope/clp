#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

"${script_dir}/install-cmake.sh"
"${script_dir}/install-go-task.sh"
"${script_dir}/install-uv.sh"
