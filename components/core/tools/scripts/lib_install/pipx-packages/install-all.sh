#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

is_sudo_from_non_root=$(( $EUID == 0 && ${SUDO_UID:-0} != 0 ))
if (( is_sudo_from_non_root )); then
    echo "Installing pipx packages to the user environment (sudo lifted)."
    exec sudo --set-home --user="$SUDO_USER" "$0" "$@"
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

"${script_dir}/install-cmake.sh"
"${script_dir}/install-go-task.sh"
"${script_dir}/install-uv.sh"
