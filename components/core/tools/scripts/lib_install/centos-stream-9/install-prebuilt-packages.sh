#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

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

# ystdlib requires CMake v3.23; ANTLR and yaml-cpp do not yet support CMake v4+.
# See also: https://github.com/y-scope/clp/issues/795
if ! command -v cmake ; then
    pipx install "cmake~=3.23"
fi

# Determine architecture for `task` release to install
rpm_arch=$(rpm --eval "%{_arch}")
case "$rpm_arch" in
    "x86_64")
        task_pkg_arch="amd64"
        ;;
    "aarch64")
        task_pkg_arch="arm64"
        ;;
    *)
        echo "Error: Unsupported architecture - $rpm_arch"
        exit 1
        ;;
esac

# Install `task`
# NOTE: We lock `task` to a version < 3.43 to avoid https://github.com/y-scope/clp/issues/872
task_pkg_path="$(mktemp -t --suffix ".rpm" task-pkg.XXXXXXXXXX)"
curl \
    --fail \
    --location \
    --output "$task_pkg_path" \
    --show-error \
    "https://github.com/go-task/task/releases/download/v3.42.1/task_linux_${task_pkg_arch}.rpm"
dnf install --assumeyes "$task_pkg_path"
rm "$task_pkg_path"
