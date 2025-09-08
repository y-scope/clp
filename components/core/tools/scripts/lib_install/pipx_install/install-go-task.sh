#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

if ! command -v pipx >/dev/null 2>&1; then
    echo "Error: pipx not found."
    exit 1
fi

if command -v task >/dev/null 2>&1; then
    version_check_script="${script_dir}/../lib_version_checks/check-go-task-version.sh"
    if ! "${version_check_script}"; then
        echo "Please uninstall go-task and then re-run the install script."
        exit 1
    fi
else
    # We lock to version 3.44.0 to avoid https://github.com/y-scope/clp-ffi-js/issues/110
    pipx install --force "go-task-bin==3.44.0"
    pipx ensurepath
fi
