#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

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
  python3-dev \
  python3-pip \
  python3-venv \
  rsync \
  software-properties-common \
  unzip

# Install `cmake`, `go-task` and `uv`
"${script_dir}/../pipx_install/install-all.sh"
