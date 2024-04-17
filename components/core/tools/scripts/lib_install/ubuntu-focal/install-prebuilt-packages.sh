#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -y \
  ca-certificates \
  checkinstall \
  cmake \
  curl \
  git \
  g++-10 \
  gcc-10 \
  libboost-filesystem-dev \
  libboost-iostreams-dev \
  libboost-program-options-dev \
  libmariadb-dev \
  libssl-dev \
  make \
  openjdk-11-jdk \
  pkg-config \
  python3 \
  python3-pip \
  zlib1g-dev
