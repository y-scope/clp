#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y \
  checkinstall \
  curl \
  libmariadb-dev \
  python3 \
  rsync \
  zstd
