#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

if ! command -v pipx >/dev/null 2>&1; then
    echo "Error: pipx is not available."
    exit 1
fi

if command -v uv >/dev/null 2>&1; then
    version_check_script="${script_dir}/../lib_version_checks/check-uv-version.sh"
    if ! script_output=$("${version_check_script}" 2>&1); then
        echo "${script_output}"
        echo "Please manually uninstall the current uv and then re-run the install script."
        exit 1
    fi
else
    pipx install --force "uv>=0.8"
    pipx ensurepath
fi

