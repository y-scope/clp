#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

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
    pkg-config \
    python3 \
    python3-dev \
    python3-pip \
    python3-venv \
    rsync \
    software-properties-common \
    unzip

# Install remaining packages through pipx
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
"${script_dir}/../pipx-packages/install-all.sh"
