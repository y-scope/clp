#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

# shellcheck source=components/core/tools/scripts/lib_install/ca-trust-pkg-opts.sh
source "$(dirname "${BASH_SOURCE[0]}")/../ca-trust-pkg-opts.sh"

dnf "${CLP_DNF_CA_OPTS[@]}" install -y \
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
