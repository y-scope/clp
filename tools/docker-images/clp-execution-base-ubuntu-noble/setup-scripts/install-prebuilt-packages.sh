#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u
set -o pipefail

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -y \
  ca-certificates \
  checkinstall \
  curl \
  libcurl4 \
  libmariadb-dev \
  libssl-dev \
  python3 \
  rsync \
  zstd

# Clean up apt cache
apt-get clean
rm -rf /var/lib/apt/lists/*