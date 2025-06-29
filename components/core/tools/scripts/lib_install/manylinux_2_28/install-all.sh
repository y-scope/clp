#!/usr/bin/env bash

set -eu
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

"${script_dir}/install-prebuilt-packages.sh"
"${script_dir}/install-packages-from-source.sh"

# TODO: https://github.com/y-scope/clp/issues/795
"${script_dir}/../check-cmake-version.sh"
