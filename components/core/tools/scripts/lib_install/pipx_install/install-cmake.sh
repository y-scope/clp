#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

if ! command -v pipx >/dev/null 2>&1; then
    echo "Error: pipx is not available."
    exit 1
fi

if command -v cmake >/dev/null 2>&1; then
    version_check_script="${script_dir}/../lib_version_checks/check-cmake-version.sh"
    if ! script_output=$("${version_check_script}" 2>&1); then
        echo "${script_output}"
        echo "Please manually uninstall the current cmake and then re-run the install script."
        exit 1
    fi
else
    # ystdlib requires CMake v3.23; ANTLR and yaml-cpp do not yet support CMake v4+.
    # See also: https://github.com/y-scope/clp/issues/795
    pipx install --force "cmake>=3.23,<3.24"
    pipx ensurepath
fi
