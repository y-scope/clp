#!/bin/bash

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y \
  ca-certificates \
  checkinstall \
  cmake \
  curl \
  build-essential \
  gcc-8 \
  g++-8 \
  libboost-filesystem-dev \
  libboost-iostreams-dev \
  libboost-program-options-dev \
  libssl-dev \
  python3 \
  python3-pip \
  rsync \
  software-properties-common

# Install latest version of git
add-apt-repository -y ppa:git-core/ppa
apt-get update
apt-get install -y git

# Install latest version of CMAKE
curl -fsSl https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
apt-get update
apt-get install -y cmake

# Switch to gcc-8
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 800 --slave /usr/bin/g++ g++ /usr/bin/g++-8
