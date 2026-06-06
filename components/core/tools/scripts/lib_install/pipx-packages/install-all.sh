#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

# Each install-*.sh below pipx-installs a tool into the pipx bin dir and then
# immediately invokes it (to verify the installed version). `pipx ensurepath`
# updates the user's shell rc for FUTURE shells but does NOT modify the
# current process's PATH, so a fresh `command -v cmake` (or `task`, or `uv`)
# wouldn't find the binary and the scripts would fail with "command not
# found". We prepend the pipx bin dir here so every child install-*.sh inherits
# the right search path before invoking the just-installed tool.
pipx_bin_dir="${PIPX_BIN_DIR:-${HOME}/.local/bin}"
export PATH="${pipx_bin_dir}:${PATH}"

"${script_dir}/install-cmake.sh"
"${script_dir}/install-go-task.sh"
"${script_dir}/install-uv.sh"
