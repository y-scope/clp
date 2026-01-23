#!/usr/bin/env bash

set -o errexit
set -o nounset
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

# Install remaining packages through pipx
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
"${script_dir}/../pipx-packages/install-all.sh"
