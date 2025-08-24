#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

dnf install -y \
    cmake \
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
export PIPX_HOME=/opt/pipx
export PIPX_BIN_DIR=/usr/bin

# Install CMake v3.31.x as ANTLR and yaml-cpp do not yet support CMake v4+.
# See also: https://github.com/y-scope/clp/issues/795
if ! command -v cmake ; then
    pipx install "cmake~=3.31"
fi

# Install a version of `task` < 3.43 to avoid https://github.com/y-scope/clp/issues/872
if ! command -v task ; then
    pipx install "go-task-bin>=3.40,<3.43"
fi
