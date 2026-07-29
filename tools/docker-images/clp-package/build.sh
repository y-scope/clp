#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
repo_root="${script_dir}/../../.."

. "${script_dir}/../utils.sh"

ubuntu_version_codename="jammy"
if [[ -f /etc/os-release ]]; then
    host_codename="$(. /etc/os-release && echo "$VERSION_CODENAME")"
    if [[ -n "$host_codename" ]]; then
        ubuntu_version_codename="$host_codename"
    fi
fi

build_clp_image \
    "clp-package" \
    "${repo_root}" \
    "${script_dir}/Dockerfile" \
    --build-arg "UBUNTU_VERSION_CODENAME=${ubuntu_version_codename}"
