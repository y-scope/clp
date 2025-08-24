#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -y \
  ca-certificates \
  checkinstall \
  curl \
  build-essential \
  git \
  jq \
  libcurl4 \
  libcurl4-openssl-dev \
  liblzma-dev \
  libmariadb-dev \
  libssl-dev \
  openjdk-11-jdk \
  pipx \
  pkg-config \
  python3 \
  python3-pip \
  python3-venv \
  software-properties-common \
  unzip

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
