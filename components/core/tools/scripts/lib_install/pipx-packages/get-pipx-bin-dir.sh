#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

# When running under sudo, pipx and pipx-managed packages are installed into the original user
# environment. Invoke pipx as that user to query the correct bin directory.
pipx_bin_dir_cmd_args=()
is_sudo_from_non_root=$(( EUID == 0 && ${SUDO_UID:-0} != 0 ))
if (( is_sudo_from_non_root )); then
    pipx_bin_dir_cmd_args+=(sudo --user="${SUDO_USER}" --set-home --)
fi
pipx_bin_dir_cmd_args+=(pipx environment --value PIPX_BIN_DIR)

"${pipx_bin_dir_cmd_args[@]}" || {
    echo "Failed to determine pipx bin directory via pipx environment (require version ≥ 1.7.0)."
    exit 1
}
