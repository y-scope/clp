#!/usr/bin/env bash

set -eu
set -o pipefail

dnf install -y \
    gcc-c++ \
    java-11-openjdk \
    jq \
    libcurl-devel \
    mariadb-connector-c-devel \
    openssl-devel \
    zlib-devel \
    zlib-static

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
task_pkg_path=$(mktemp -t --suffix ".rpm" task-pkg.XXXXXXXXXX) || exit 1
curl \
    --fail \
    --location \
    --output "$task_pkg_path" \
    --show-error \
    "https://github.com/go-task/task/releases/download/v3.42.1/task_linux_${task_pkg_arch}.rpm"
dnf install --assumeyes "$task_pkg_path"
rm "$task_pkg_path"

# Downgrade to CMake v3 to work around https://github.com/y-scope/clp/issues/795
pipx uninstall cmake
pipx install cmake~=3.31
