#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

readonly required_version_major_min=0
readonly required_version_minor_min=8
readonly required_version_min="${required_version_major_min}.${required_version_minor_min}"

# Prepend the pipx bin directory to PATH so pipx-installed uv takes precedence.
pipx_bin_dir="$("${script_dir}/../pipx-packages/get-pipx-bin-dir.sh")"
export PATH="${pipx_bin_dir}:${PATH}"

pipx uninstall uv >/dev/null 2>&1 || true
pipx install "uv>=${required_version_min}"
uv_bin="$(command -v uv)"

case "$uv_bin" in
    "$pipx_bin_dir"/*)
        ;;
    *)
        echo "uv is not found or not from pipx: ${uv_bin}"
        exit 1
        ;;
esac

installed_version=$(${uv_bin} self version --output-format json | jq --raw-output ".version")
IFS=. read -r installd_version_major installed_version_minor _ <<<"${installed_version}"

if (("${installd_version_major}" == "${required_version_major_min}" && \
    "${installed_version_minor}" < "${required_version_minor_min}")); then
    echo "Error: uv version ${installed_version} is unsupported (require version " \
        "≥ ${required_version_min})."
    echo "pipx failed to install the required version of uv."
    echo "To uninstall, run:"
    echo "  pipx uninstall uv"

    exit 1
fi

echo "uv version ${installed_version} installed at ${uv_bin} satisfies version requirements."
