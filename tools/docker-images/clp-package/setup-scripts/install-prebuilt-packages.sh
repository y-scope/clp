#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -y \
  ca-certificates \
  curl \
  libcurl4 \
  libmariadb3 \
  python3
