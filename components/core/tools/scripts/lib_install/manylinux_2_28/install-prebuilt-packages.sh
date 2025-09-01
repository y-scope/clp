#!/usr/bin/env bash

# Exit on any error, use of undefined variables, or failure within a pipeline
set -euo pipefail

dnf install -y \
    gcc-c++ \
    java-11-openjdk \
    jq \
    libcurl-devel \
    mariadb-connector-c-devel \
    openssl-devel \
    zlib-devel \
    zlib-static

# Install `cmake`, `go-task` and `uv`
"${script_dir}/../pipx_install/install-all.sh"
