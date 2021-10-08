#!/bin/bash

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y \
  ca-certificates \
  checkinstall \
  cmake \
  build-essential \
  libboost-filesystem-dev \
  libboost-iostreams-dev \
  libboost-program-options-dev \
  libssl-dev \
  pkg-config \
  rsync \
  wget \
  zlib1g-dev
