#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

if ! command -v pipx >/dev/null 2>&1; then
    echo "Error: pipx not found."
    exit 1
fi

# We lock to version 3.44.0 to avoid https://github.com/y-scope/clp-ffi-js/issues/110
readonly required_version="3.44.0"

package_preinstalled=0
if ! command -v task >/dev/null 2>&1; then
    package_preinstalled=1
    pipx install --force "go-task-bin==${required_version}"
    pipx ensurepath
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
task_version=$(task --silent --taskfile "${script_dir}/print-go-task-version.yaml")
if [[ "${task_version}" != "${required_version}" ]]; then
    echo "Error: Task version ${task_version} is currently unsupported (must be" \
        "${required_version})."

    if ((0 == "${package_preinstalled}")); then
        echo "Please uninstall Task and then re-run the install script."
    else
        echo "pipx failed to install the required version of Task."
        echo "To uninstall, run:"
        echo "  pipx uninstall go-task-bin"
    fi

    exit 1
fi
