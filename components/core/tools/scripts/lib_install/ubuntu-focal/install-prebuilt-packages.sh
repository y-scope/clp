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
  g++ \
  g++-10 \
  gcc \
  gcc-10 \
  git \
  jq \
  libcurl4 \
  libcurl4-openssl-dev \
  liblzma-dev \
  libmariadb-dev \
  libssl-dev \
  make \
  openjdk-11-jdk \
  pkg-config \
  python3 \
  python3-pip \
  zlib1g-dev

# Add alternatives for cpp-10, gcc-10, and g++-10
# NOTE: We use a low priority to avoid affecting the prioritization of any existing alternatives
# on the user's system.
update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-10 0 \
  --slave /usr/share/man/man1/cc.1.gz cc.1.gz /usr/share/man/man1/gcc.1.gz
update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-10 0 \
  --slave /usr/share/man/man1/c++.1.gz c++.1.gz /usr/share/man/man1/g++.1.gz
update-alternatives --install /lib/cpp cpp /usr/bin/cpp-10 0
