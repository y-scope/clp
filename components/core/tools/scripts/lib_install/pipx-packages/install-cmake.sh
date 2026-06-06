#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

if ! command -v pipx >/dev/null 2>&1; then
    echo "Error: pipx not found."
    exit 1
fi

if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq not found."
    echo "Install jq before running this script."
    exit 1
fi

readonly required_version_major_min=3
readonly required_version_minor_min=23
readonly required_version_min="${required_version_major_min}.${required_version_minor_min}"
readonly required_version_major_max=3
readonly required_version_major_max_plus_1=$((required_version_major_max + 1))

# True iff the cmake currently on PATH satisfies our version bounds.
# Used so we treat "preinstalled but wrong-version cmake" the same as
# "no cmake at all": both cases reach the pipx install branch.
_cmake_version_acceptable() {
    command -v cmake >/dev/null 2>&1 || return 1
    local m n
    m=$(cmake -E capabilities 2>/dev/null | jq --raw-output ".version.major" 2>/dev/null) \
        || return 1
    n=$(cmake -E capabilities 2>/dev/null | jq --raw-output ".version.minor" 2>/dev/null) \
        || return 1
    [[ -n "${m}" && -n "${n}" ]] || return 1
    if ((m < required_version_major_min)); then return 1; fi
    if ((m == required_version_major_min && n < required_version_minor_min)); then return 1; fi
    if ((m >= required_version_major_max_plus_1)); then return 1; fi
    return 0
}

package_preinstalled=0
if ! _cmake_version_acceptable; then
    package_preinstalled=1
    # ystdlib requires CMake v3.23; ANTLR and yaml-cpp do not yet support CMake v4+
    # (see https://github.com/y-scope/clp/issues/795).
    pipx install --force "cmake>=${required_version_min},<${required_version_major_max_plus_1}"
    pipx ensurepath
    # `pipx ensurepath` updates the user's shell rc for FUTURE shells. To make
    # the pipx-installed cmake available NOW (so the version recheck below
    # picks it up — and so the parent install-all.sh's later steps use it
    # instead of any too-new cmake at /opt/homebrew/bin/cmake), prepend the
    # pipx bin to PATH for this script's process tree.
    pipx_bin_dir="${PIPX_BIN_DIR:-${HOME}/.local/bin}"
    export PATH="${pipx_bin_dir}:${PATH}"
fi

installed_version=$(cmake -E capabilities | jq --raw-output ".version.string")
installed_version_major=$(cmake -E capabilities | jq --raw-output ".version.major")
installed_version_minor=$(cmake -E capabilities | jq --raw-output ".version.minor")

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
