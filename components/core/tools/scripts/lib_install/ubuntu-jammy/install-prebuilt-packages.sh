#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -y \
  ca-certificates \
  checkinstall \
  cmake \
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
  pkg-config \
  python3 \
  python3-pip \
  python3-venv \
  software-properties-common \
  unzip

# Install `task`
# NOTE: We lock `task` to a version < 3.43 to avoid https://github.com/y-scope/clp/issues/872
task_pkg_arch=$(dpkg --print-architecture)
task_pkg_path="$(mktemp -t --suffix ".deb" task-pkg.XXXXXXXXXX)"
curl \
    --fail \
    --location \
    --output "$task_pkg_path" \
    --show-error \
    "https://github.com/go-task/task/releases/download/v3.42.1/task_linux_${task_pkg_arch}.deb"
dpkg --install "$task_pkg_path"
rm "$task_pkg_path"
