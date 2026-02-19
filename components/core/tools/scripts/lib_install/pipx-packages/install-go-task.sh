#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

if ! command -v pipx >/dev/null 2>&1; then
    echo "Error: pipx not found."
    exit 1
fi

readonly required_version_major_min=3
readonly required_version_minor_min=48
readonly required_version_min="${required_version_major_min}.${required_version_minor_min}"

package_preinstalled=0
if ! command -v task >/dev/null 2>&1; then
    package_preinstalled=1
    pipx install --force "go-task-bin>=${required_version_min}"
    pipx ensurepath
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
installed_version=$(task --silent --taskfile "${script_dir}/print-go-task-version.yaml")
IFS=. read -r installed_version_major installed_version_minor _ <<<"${installed_version}"

is_major_older=$(( installed_version_major < required_version_major_min ))
is_minor_older=$(( installed_version_major == required_version_major_min
    && installed_version_minor < required_version_minor_min ))
if (( is_major_older || is_minor_older )); then
    echo "Error: Task version ${installed_version} is unsupported (requires version" \
        ">= ${required_version_min})."

    if ((0 == "${package_preinstalled}")); then
        echo "Please uninstall Task and then re-run the install script."
    else
        echo "pipx failed to install the required version of Task."
        echo "To uninstall, run:"
        echo "  pipx uninstall go-task-bin"
    fi

    exit 1
fi
