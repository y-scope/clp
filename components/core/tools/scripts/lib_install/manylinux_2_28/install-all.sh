#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

"${script_dir}/install-prebuilt-packages.sh"

pipx_bin_dir="$("${script_dir}/../pipx-packages/get-pipx-bin-dir.sh")"
export PATH="${pipx_bin_dir}:${PATH}"
"${script_dir}/install-packages-from-source.sh"
