#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

if ! command -v pipx >/dev/null 2>&1; then
    echo "Error: pipx not found."
    exit 1
fi

# NOTE: ystdlib requires CMake v3.23.
readonly required_version_major_min=3
readonly required_version_minor_min=23
readonly required_version_min="${required_version_major_min}.${required_version_minor_min}"

# NOTE: CLP builds with CMake v4+, so the check below accepts it, but we install v3 to keep
# adopting v4 a deliberate choice rather than a side effect of installing the newest version.
readonly installed_version_major_max_plus_1=4

package_preinstalled=0
if ! command -v cmake >/dev/null 2>&1; then
    package_preinstalled=1
    pipx install --force "cmake>=${required_version_min},<${installed_version_major_max_plus_1}"
    pipx ensurepath

    # NOTE: `pipx ensurepath` updates shell startup files but not this process, so the CMake we
    # just installed may still not resolve below. `pipx environment` would give us the
    # application directory, but it doesn't exist in pipx v1.0 (Ubuntu 22.04), so fall back to
    # pipx's default.
    if ! command -v cmake >/dev/null 2>&1; then
        PATH="${PIPX_BIN_DIR:-${HOME}/.local/bin}:${PATH}"
        export PATH
    fi
fi

installed_version=$(cmake -E capabilities | jq --raw-output ".version.string")
installed_version_major=$(cmake -E capabilities | jq --raw-output ".version.major")
installed_version_minor=$(cmake -E capabilities | jq --raw-output ".version.minor")

if (("${installed_version_major}" < "${required_version_major_min}")) \
    || (("${installed_version_major}" == "${required_version_major_min}" && \
    "${installed_version_minor}" < "${required_version_minor_min}")); then
    echo "Error: CMake version ${installed_version} is unsupported (require version" \
        "≥ ${required_version_min})."

    if ((0 == "${package_preinstalled}")); then
        echo "Please uninstall CMake and then re-run the install script."
    else
        echo "pipx failed to install the required version of CMake."
        echo "To uninstall, run:"
        echo "  pipx uninstall cmake"
    fi

    exit 1
fi
