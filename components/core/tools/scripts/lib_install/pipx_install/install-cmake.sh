#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

if ! command -v pipx >/dev/null 2>&1; then
    echo "Error: pipx not found."
    exit 1
fi

if command -v cmake >/dev/null 2>&1; then
    version_check_script="${script_dir}/../lib_version_checks/check-cmake-version.sh"
    if ! "${version_check_script}"; then
        echo "Please uninstall cmake and then re-run the install script."
        exit 1
    fi
else
    # ystdlib requires CMake v3.23; ANTLR and yaml-cpp do not yet support CMake v4+
    # (see https://github.com/y-scope/clp/issues/795).
    pipx install --force "cmake>=3.23,<4"
    pipx ensurepath
fi
