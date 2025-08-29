#!/usr/bin/env bash

# Exit on any error, use of undefined variables, or failure within a pipeline
set -euo pipefail

apk update && apk add --no-cache \
    bzip2-dev \
    bzip2-static \
    curl-dev \
    jq \
    mariadb-connector-c-dev \
    openjdk11-jdk \
    openssl-dev \
    zlib-dev \
    zlib-static

# Install `cmake`
# ystdlib requires CMake v3.23; ANTLR and yaml-cpp do not yet support CMake v4+.
# See also: https://github.com/y-scope/clp/issues/795
pipx uninstall cmake || true
if ! command -v cmake ; then
    pipx install "cmake~=3.23"
fi

# Install `task`
# NOTE: We lock `task` to a version < 3.43 to avoid https://github.com/y-scope/clp/issues/872
pipx uninstall go-task-bin || true
if ! command -v task ; then
    pipx install "go-task-bin>=3.40,<3.43"
fi

# Install `uv`
pipx uninstall uv || true
if ! command -v uv ; then
    pipx install "uv>=0.8"
fi
