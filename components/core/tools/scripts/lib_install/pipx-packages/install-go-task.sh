#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

if ! command -v pipx >/dev/null 2>&1; then
    echo "Error: pipx not found."
    exit 1
fi

# We lock to version 3.44.0 to avoid https://github.com/y-scope/clp/issues/1352
readonly required_version="3.44.0"

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

go_task_bin="$(command -v task 2>/dev/null || true)"
if [ -n "${go_task_bin}" ]; then
    package_preinstalled=0
    echo "Preinstalled Task found at: ${go_task_bin}"
else
    package_preinstalled=1
    pipx install --force "go-task-bin==${required_version}"
    go_task_bin=$("${script_dir}/find-pipx-bin.sh" go-task-bin task)
    echo "Pipx Task installed at: ${go_task_bin}"
fi

installed_version=$(${go_task_bin} --silent --taskfile "${script_dir}/print-go-task-version.yaml")
if [[ "${installed_version}" != "${required_version}" ]]; then
    echo "Error: Task version ${installed_version} is currently unsupported (must be" \
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

echo "Task version ${installed_version} installed at ${go_task_bin} satisfies version requirements."
