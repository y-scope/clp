#!/bin/bash

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y \
  checkinstall \
  curl \
  python3 \
  rsync \
  zstd
