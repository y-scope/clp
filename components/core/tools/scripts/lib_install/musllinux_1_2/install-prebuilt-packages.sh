#!/usr/bin/env bash

set -eu
set -o pipefail

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

# Install `task`
# NOTE: We lock `task` to a version < 3.43 to avoid https://github.com/y-scope/clp/issues/872
VERSION=3.42.1
ARCH=$(uname -m)
case "$ARCH" in
    x86_64) ARCH=amd64 ;;
    aarch64) ARCH=arm64 ;;
    *) echo "Unsupported architecture: $ARCH"; exit 1 ;;
esac

wget -O /tmp/task.tar.gz \
    "https://github.com/go-task/task/releases/download/v${VERSION}/task_linux_${ARCH}.tar.gz"

tar -C /usr/local/bin -xzf /tmp/task.tar.gz task
chmod +x /usr/local/bin/task
rm /tmp/task.tar.gz

# Downgrade to CMake v3 to work around https://github.com/y-scope/clp/issues/795
pipx uninstall cmake
pipx install cmake~=3.31
