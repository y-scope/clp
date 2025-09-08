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
# NOTE: We lock `task` to version 3.44.0 to avoid https://github.com/y-scope/clp-ffi-js/issues/110
task_pkg_path="$(mktemp -t --suffix ".rpm" task-pkg.XXXXXXXXXX)"
curl \
    --fail \
    --location \
    --output "$task_pkg_path" \
    --show-error \
    "https://github.com/go-task/task/releases/download/v3.44.0/task_linux_${task_pkg_arch}.rpm"
dnf install --assumeyes "$task_pkg_path"
rm "$task_pkg_path"
