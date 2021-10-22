#!/bin/bash

yum install -y \
  bzip2 \
  centos-release-scl \
  gcc \
  gcc-c++ \
  make \
  openssl-devel \
  openssl-static \
  python3 \
  rsync \
  wget \
  zlib-static

# NOTE: We can only install the devtoolset after centos-release-scl
yum install -y devtoolset-7 \
  rh-git227
