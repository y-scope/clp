#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)

cUsage="Usage: ${BASH_SOURCE[0]} <clp-package-dir>"
if [ "$#" -lt 1 ]; then
    echo "$cUsage"
    exit
fi
clp_package_dir=$1

venv_dir=${script_dir}/.venv
if [ ! -d "${venv_dir}" ]; then
    echo "Setting up Python venv in '${venv_dir}'..."
    python3 -m venv "${script_dir}/.venv"
fi
source "${script_dir}/.venv/bin/activate"

echo "Installing required Python packages..."
pip3 install -r "${script_dir}/requirements.txt"

echo "Generating config files corresponding to user-configured properties..."
python3 "${script_dir}/init.py" \
    --clp-package-dir "${clp_package_dir}" \
    --output-file "${script_dir}/../.env"
