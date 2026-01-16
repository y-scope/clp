#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

"${script_dir}/install-prebuilt-packages.sh"
"${script_dir}/../pipx-packages/install-all.sh"

# Manually perform pipx ensurepath --prepend inside script
export PATH="$(pipx environment --value PIPX_BIN_DIR 2>/dev/null):${PATH}"

"${script_dir}/install-packages-from-source.sh"
