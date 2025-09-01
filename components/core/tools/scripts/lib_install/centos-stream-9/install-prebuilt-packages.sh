#!/usr/bin/env bash

# Exit on any error, use of undefined variables, or failure within a pipeline
set -euo pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

dnf install -y \
    diffutils \
    gcc-c++ \
    git \
    java-11-openjdk \
    jq \
    libarchive-devel \
    libcurl-devel \
    libzstd-devel \
    lz4-devel \
    make \
    mariadb-connector-c-devel \
    openssl-devel \
    python3-pip \
    unzip

if ! command -v pipx >/dev/null 2>&1; then
    python3 -m pip install pipx
fi

# Install `cmake`, `go-task` and `uv`
"${script_dir}/../pipx_install/install-all.sh"
