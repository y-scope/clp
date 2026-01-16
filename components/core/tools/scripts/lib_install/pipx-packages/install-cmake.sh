#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

if ! command -v pipx >/dev/null 2>&1; then
    echo "Error: pipx not found."
    exit 1
fi

readonly required_version_major_min=3
readonly required_version_minor_min=23
readonly required_version_min="${required_version_major_min}.${required_version_minor_min}"
readonly required_version_major_max=3
readonly required_version_major_max_plus_1=$((required_version_major_max + 1))

cmake_bin="$(command -v cmake 2>/dev/null || true)"
if [ -n "${cmake_bin}" ]; then
    package_preinstalled=0
    echo "Preinstalled CMake found at: ${cmake_bin}"
else
    package_preinstalled=1
    # ystdlib requires CMake v3.23; ANTLR and yaml-cpp do not yet support CMake v4+
    # (see https://github.com/y-scope/clp/issues/795).
    pipx install --force "cmake>=${required_version_min},<${required_version_major_max_plus_1}"
    script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
    cmake_bin=$("${script_dir}/find-pipx-bin.sh" cmake cmake)
    echo "Pipx CMake installed at: ${cmake_bin}"
fi

installed_version=$("${cmake_bin}" -E capabilities | jq --raw-output ".version.string")
installed_version_major=$("${cmake_bin}" -E capabilities | jq --raw-output ".version.major")
installed_version_minor=$("${cmake_bin}" -E capabilities | jq --raw-output ".version.minor")

# ystdlib requires CMake v3.23; ANTLR and yaml-cpp do not yet support CMake v4+
# (see https://github.com/y-scope/clp/issues/795).
if (("${installed_version_major}" < "${required_version_major_min}")) \
    || (("${installed_version_major}" == "${required_version_major_min}" && \
    "${installed_version_minor}" < "${required_version_minor_min}")) \
    || (("${installed_version_major}" >= "${required_version_major_max_plus_1}")); then
    echo "Error: CMake version ${installed_version} is unsupported (require" \
        "${required_version_min} ≤ version < ${required_version_major_max_plus_1})."

    if ((0 == "${package_preinstalled}")); then
        echo "Please uninstall CMake and then re-run the install script."
    else
        echo "pipx failed to install the required version of CMake."
        echo "To uninstall, run:"
        echo "  pipx uninstall cmake"
    fi

    exit 1
fi

echo "CMake version ${installed_version} installed at ${cmake_bin} satisfies version requirements."
