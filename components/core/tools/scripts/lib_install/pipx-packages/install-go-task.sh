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

pipx uninstall go-task-bin >/dev/null 2>&1 || true
pipx install "go-task-bin==${required_version}"

hash -d task 2>/dev/null || true
installed_version=$(task --silent --taskfile "${script_dir}/print-go-task-version.yaml")

if [[ "${installed_version}" != "${required_version}" ]]; then
    echo "Error: Task version ${installed_version} is currently unsupported (must be " \
        "${required_version})."
    echo "pipx failed to install the required version of Task."
    echo "To uninstall, run:"
    echo "  pipx uninstall go-task-bin"

    exit 1
fi

echo "Task version ${installed_version} satisfies version requirements."
