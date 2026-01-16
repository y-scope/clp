#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

readonly required_version_major_min=1
readonly required_version_minor_min=8
readonly required_version_min="${required_version_major_min}.${required_version_minor_min}"

# Get python3 path
python3_bin="$(command -v python3 2>/dev/null || true)"
pipx_bin="$(command -v pipx 2>/dev/null || true)"
if [ -n "${pipx_bin}" ]; then
    # Prefer using pipx default python3 if available
    pipx_python3_bin"$("${pipx_bin}" environment --value PIPX_DEFAULT_PYTHON 2>/dev/null || true)"
    if [ -n "${pipx_python3_bin}" ]; then
        python3_bin="${pipx_python3_bin}"
    fi
fi

# Attempt to perform pipx upgrade via pip install regardless of the current pipx version
PIP_USER_FLAG=""
if [ "$(id -u)" -ne 0 ]; then
    PIP_USER_FLAG="--user"
fi
if [ -n "${python3_bin}" ]; then
    pipx_upgrade_cmd = "${python3_bin} -m pip install ${PIP_USER_FLAG} -U pipx"
    ${pipx_upgrade_cmd} >/dev/null 2>&1 || true
fi

# Find pipx path and validate version
pipx_bin="$(command -v pipx 2>/dev/null || true)"
if [ -z "${pipx_bin:-}" ]; then
    echo "Error: failed to automatically install pipx. Please install it manually."
    exit 1
fi

installed_version="$("${pipx_bin}" --version)"
IFS=. read -r installd_version_major installed_version_minor _ <<<"${installed_version}"

if (("${installed_version_major}" < "${required_version_major_min}")) \
    || (("${installed_version_major}" == "${required_version_major_min}" && \
    "${installed_version_minor}" < "${required_version_minor_min}")); then
    echo "Error: pipx version ${installed_version} is unsupported (require version" \
        "≥ ${required_version_min})."
    exit 1
fi

echo "pipx version ${installed_version} installed at ${pipx_bin} satisfies version requirements."

# Manually perform pipx ensurepath --prepend
echo "Prepending pipx binary directory to \$PATH."
PIPX_BIN_DIR="$("${pipx_bin}" environment --value PIPX_BIN_DIR 2>/dev/null || true)"
if [ -n "${PIPX_BIN_DIR}" ]; then
    export PATH="${PIPX_BIN_DIR}:${PATH}"
else
    echo "Error: failed to locate pipx binary directory; pipx may not be installed correctly."
fi
