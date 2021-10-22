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
  python3 \
  rsync \
  software-properties-common \
  wget

# Install latest version of git
add-apt-repository -y ppa:git-core/ppa
apt-get update
apt-get install -y git