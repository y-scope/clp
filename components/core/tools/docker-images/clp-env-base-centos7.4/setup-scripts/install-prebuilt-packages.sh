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
  python3-pip \
  rsync \
  zlib-static

# Install packages from CentOS' software collections repository (centos-release-scl)
yum install -y \
  devtoolset-7 \
  rh-git227
