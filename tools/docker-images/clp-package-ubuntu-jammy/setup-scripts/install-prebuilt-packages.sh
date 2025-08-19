#!/usr/bin/env bash

set -eu
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
