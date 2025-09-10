#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

if ! command -v pipx >/dev/null 2>&1; then
    echo "Error: pipx not found."
    exit 1
fi

package_preinstalled=0
if ! command -v uv >/dev/null 2>&1; then
    package_preinstalled=1
    pipx install --force "uv>=0.8"
    pipx ensurepath
fi

uv_version=$(uv self version --output-format json | jq --raw-output ".version")
IFS=. read -r uv_major_version uv_minor_version _ <<<"${uv_version}"

if (("${uv_major_version}" == 0 && "${uv_minor_version}" < 8)); then
    echo "Error: uv version ${uv_version} is unsupported (require version ≥ 0.8)."

    if ((0 == "${package_preinstalled}")); then
        echo "Please uninstall uv and then re-run the install script."
    else
        echo "pipx failed to install the required version of uv."
        echo "To uninstall, run:"
        echo "  pipx uninstall uv"
    fi

    exit 1
fi
