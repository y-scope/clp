#!/usr/bin/env bash

# Exit on any error, use of undefined variables, or failure within a pipeline
set -euo pipefail

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

if ! command -v pipx; then
    python3 -m pip install pipx
fi

if [ "$(id -u)" -eq 0 ]; then
    # Running as root: install pipx softwares into system directories
    export PIPX_HOME=/opt/_internal/pipx
    export PIPX_BIN_DIR=/usr/local/bin
fi
pipx ensurepath

# Install `cmake`
# ystdlib requires CMake v3.23; ANTLR and yaml-cpp do not yet support CMake v4+.
# See also: https://github.com/y-scope/clp/issues/795
if ! command -v cmake ; then
    pipx install "cmake~=3.23"
fi

# Install `task`
# NOTE: We lock `task` to a version < 3.43 to avoid https://github.com/y-scope/clp/issues/872
if ! command -v task ; then
    pipx install "go-task-bin>=3.40,<3.43"
fi

# Install `uv`
if ! command -v uv ; then
    pipx install "uv>=0.8"
fi
