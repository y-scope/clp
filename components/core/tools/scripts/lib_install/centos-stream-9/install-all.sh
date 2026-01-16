#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

"${script_dir}/install-prebuilt-packages.sh"

# Install pipx packages and explicitly prepend the pipx bin directory to PATH for tooling visibility
"${script_dir}/../pipx-packages/install-all.sh"
export PATH="$("${script_dir}/../pipx-packages/get-pipx-bin-dir.sh"):${PATH}"

"${script_dir}/install-packages-from-source.sh"
