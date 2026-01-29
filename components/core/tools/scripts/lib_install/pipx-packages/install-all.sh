#!/usr/bin/env bash
# Entrypoint for installing all required pipx packages. Do not invoke individual tool install
# scripts directly.

set -o errexit
set -o nounset
set -o pipefail

is_sudo_from_non_root=$(( EUID == 0 && ${SUDO_UID:-0} != 0 ))
if (( is_sudo_from_non_root )); then
    echo "Installing pipx packages to the user environment (sudo lifted)."
    exec sudo --preserve-env --set-home --user="$SUDO_USER" \
      /usr/bin/env bash "${BASH_SOURCE[0]}" "$@"
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

# In user mode, ensure ~/.local/bin is on PATH so a user-installed pipx is discoverable.
if (( EUID != 0 )); then
    export PATH="${HOME}/.local/bin:${PATH}"
fi
"${script_dir}/install-pipx.sh"

# Prepend the pipx bin directory to PATH so pipx-installed build tools take precedence in the
# following installation and version-check scripts. This step is required as the following scripts
# would behave unexpectedly otherwise.
pipx_bin_dir="$("${script_dir}/get-pipx-bin-dir.sh")"
export PATH="${pipx_bin_dir}:${PATH}"

"${script_dir}/install-cmake.sh"
"${script_dir}/install-go-task.sh"
"${script_dir}/install-uv.sh"
