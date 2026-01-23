#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

readonly required_version_major_min=1
readonly required_version_minor_min=7
readonly required_version_min="${required_version_major_min}.${required_version_minor_min}"

# Install pipx if missing, or upgrade it to the latest available version.
if [ "$(uname -s)" = "Darwin" ]; then
    # Macos
    brew install pipx || true
    brew upgrade pipx
else
    pipx_python3_bin="$(pipx environment --value PIPX_DEFAULT_PYTHON 2>/dev/null || true)"
    if [ -n "${pipx_python3_bin}" ]; then
        # Manylinux / Musllinux: use pipx default python3
        python3_bin="${pipx_python3_bin}"
    else
        # Ubuntu / Centos
        python3_bin="$(command -v python3)"
    fi

    PIP_USER_FLAG=""
    if (( EUID != 0 )); then
        PIP_USER_FLAG="--user"
    fi
    pipx_install_or_upgrade_cmd=("${python3_bin}" -m pip install ${PIP_USER_FLAG} --upgrade pipx)
    "${pipx_install_or_upgrade_cmd[@]}" || {
        echo "Error: failed to install or upgrade pipx."
        exit 1
    }
fi

# Find the most recently installed pipx and validate version
hash -d pipx 2>/dev/null || true
if ! command -v pipx >/dev/null 2>&1; then
    echo "Error: failed to automatically install pipx. Please install it manually."
    exit 1
fi

installed_version="$(pipx --version)"
IFS=. read -r installed_version_major installed_version_minor _ <<<"${installed_version}"

if (("${installed_version_major}" < "${required_version_major_min}")) \
    || (("${installed_version_major}" == "${required_version_major_min}" && \
    "${installed_version_minor}" < "${required_version_minor_min}")); then
    echo "Error: pipx version ${installed_version} is unsupported (require version" \
        "≥ ${required_version_min})."
    exit 1
fi

echo "pipx version ${installed_version} satisfies version requirements."

# Pipx ensurepath is idempotent and will not update shell rc files if the pipx bin directory
# already appears in $PATH. Since this script may have modified $PATH earlier, we clear the $PATH
# variable so that `pipx ensurepath` always updates the rc files.
echo "Running pipx ensurepath."
current_path="${PATH}"
PATH=""
pipx ensurepath --prepend
PATH="${current_path}"
