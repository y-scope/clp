#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

brew update
brew install \
    coreutils \
    gcc \
    java11 \
    libarchive \
    llvm@16 \
    lz4 \
    mariadb-connector-c \
    pipx \
    xz \
    zstd

# Install pkg-config if it isn't already installed
# NOTE: We might expect that pkg-config is installed through brew, so trying to install it again
# would be harmless; however, in certain environments, like the macOS GitHub hosted runner,
# pkg-config is installed by other means, meaning a brew install would cause conflicts.
if ! command -v pkg-config >/dev/null 2>&1; then
    brew install pkg-config
fi

# Install remaining packages through pipx
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
"${script_dir}/../pipx-packages/install-all.sh"
