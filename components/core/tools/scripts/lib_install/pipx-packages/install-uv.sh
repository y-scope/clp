#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

if ! command -v pipx >/dev/null 2>&1; then
    echo "Error: pipx not found."
    exit 1
fi

readonly required_version_major_min=0
readonly required_version_minor_min=8
readonly required_version_min="${required_version_major_min}.${required_version_minor_min}"

package_preinstalled=0
if ! command -v uv >/dev/null 2>&1; then
    package_preinstalled=1
    pipx install --force "uv>=${required_version_min}"
    pipx ensurepath
fi

installed_version=$(uv self version --output-format json | jq --raw-output ".version")
IFS=. read -r installd_version_major installed_version_minor _ <<<"${installed_version}"

if (("${installd_version_major}" == "${required_version_major_min}" && \
    "${installed_version_minor}" < "${required_version_minor_min}")); then
    echo "Error: uv version ${installed_version} is unsupported (require version" \
        "≥ ${required_version_min})."

    if ((0 == "${package_preinstalled}")); then
        echo "Please uninstall uv and then re-run the install script."
    else
        echo "pipx failed to install the required version of uv."
        echo "To uninstall, run:"
        echo "  pipx uninstall uv"
    fi

    exit 1
fi
