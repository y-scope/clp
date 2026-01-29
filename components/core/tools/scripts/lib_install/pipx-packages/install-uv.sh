#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

readonly required_version_major_min=0
readonly required_version_minor_min=8
readonly required_version_min="${required_version_major_min}.${required_version_minor_min}"

pipx uninstall uv >/dev/null 2>&1 || true
pipx install "uv>=${required_version_min}"

# Clear any cached path so the shell resolves the updated binary
hash -d uv 2>/dev/null || true
installed_version=$(uv self version --output-format json | jq --raw-output ".version")
IFS=. read -r installed_version_major installed_version_minor _ <<<"${installed_version}"

if (("${installed_version_major}" == "${required_version_major_min}" && \
    "${installed_version_minor}" < "${required_version_minor_min}")); then
    echo "Error: uv version ${installed_version} is unsupported (require version " \
        "≥ ${required_version_min})."
    echo "pipx failed to install the required version of uv."
    echo "To uninstall, run:"
    echo "  pipx uninstall uv"

    exit 1
fi

echo "uv version ${installed_version} satisfies version requirements."
