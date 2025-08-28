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
