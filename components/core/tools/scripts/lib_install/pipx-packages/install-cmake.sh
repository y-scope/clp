#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

if ! command -v pipx >/dev/null 2>&1; then
    echo "Error: pipx not found."
    exit 1
fi

package_preinstalled=0
if ! command -v cmake >/dev/null 2>&1; then
    package_preinstalled=1
    # ystdlib requires CMake v3.23; ANTLR and yaml-cpp do not yet support CMake v4+
    # (see https://github.com/y-scope/clp/issues/795).
    pipx install --force "cmake>=3.23,<4"
    pipx ensurepath
fi

cmake_version=$(cmake -E capabilities | jq --raw-output ".version.string")
cmake_major_version=$(cmake -E capabilities | jq --raw-output ".version.major")
cmake_minor_version=$(cmake -E capabilities | jq --raw-output ".version.minor")

# ystdlib requires CMake v3.23; ANTLR and yaml-cpp do not yet support CMake v4+
# (see https://github.com/y-scope/clp/issues/795).
if (("${cmake_major_version}" < 3)) \
    || (("${cmake_major_version}" == 3 && "${cmake_minor_version}" < 23)) \
    || (("${cmake_major_version}" >= 4)); then
    echo "Error: CMake version ${cmake_version} is unsupported (require 3.23 ≤ version < 4.0)."

    if ((0 == "${package_preinstalled}")); then
        echo "Please uninstall CMake and then re-run the install script."
    else
        echo "pipx failed to install the required version of CMake."
        echo "To uninstall, run:"
        echo "  pipx uninstall cmake"
    fi

    exit 1
fi
