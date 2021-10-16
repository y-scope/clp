#!/bin/bash

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y \
  ca-certificates \
  checkinstall \
  cmake \
  build-essential \
  git \
  libboost-filesystem-dev \
  libboost-iostreams-dev \
  libboost-program-options-dev \
  libssl-dev \
  python3 \
  rsync \
  wget
