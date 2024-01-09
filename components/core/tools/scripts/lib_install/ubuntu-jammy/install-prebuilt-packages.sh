#!/usr/bin/env bash

apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y \
  ca-certificates \
  checkinstall \
  cmake \
  curl \
  build-essential \
  git \
  libboost-filesystem-dev \
  libboost-iostreams-dev \
  libboost-program-options-dev \
  libmariadb-dev \
  libssl-dev \
  openjdk-11-jdk \
  pkg-config \
  python3 \
  python3-pip \
  rsync \
  software-properties-common
