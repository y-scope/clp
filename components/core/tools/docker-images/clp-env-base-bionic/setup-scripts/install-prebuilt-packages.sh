#!/bin/bash

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y \
  ca-certificates \
  checkinstall \
  cmake \
  build-essential \
  software-properties-common \
  libboost-filesystem-dev \
  libboost-iostreams-dev \
  libboost-program-options-dev \
  libssl-dev \
  python3 \
  rsync \
  wget

# Install latest version of git
add-apt-repository -y ppa:git-core/ppa
apt update
apt install -y git