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

# ystdlib requires CMake v3.23; ANTLR and yaml-cpp do not yet support CMake v4+.
# See also: https://github.com/y-scope/clp/issues/795
pipx uninstall cmake
if ! command -v cmake ; then
    pipx install "cmake~=3.23"
fi

# Determine architecture for `task` release to install
arch=$(uname -m)
case "$arch" in
    "x86_64")
        task_pkg_arch="amd64"
        ;;
    "aarch64")
        task_pkg_arch="arm64"
        ;;
    *)
        echo "Error: Unsupported architecture - $arch"
        exit 1
        ;;
esac

# Install `task`
# NOTE: We lock `task` to a version < 3.43 to avoid https://github.com/y-scope/clp/issues/872
task_pkg_path=$(mktemp -t task-pkg.XXXXXXXXXX).tar.gz || exit 1
curl \
    --fail \
    --location \
    --output "$task_pkg_path" \
    --show-error \
    "https://github.com/go-task/task/releases/download/v3.42.1/task_linux_${task_pkg_arch}.tar.gz"
tar -C /usr/local/bin -xzf "$task_pkg_path" task
chmod +x /usr/local/bin/task
rm "$task_pkg_path"
