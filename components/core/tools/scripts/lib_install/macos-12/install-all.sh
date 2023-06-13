#!/usr/bin/env bash

# Work around GitHub actions having a preinstalled 2to3
# See https://github.com/actions/setup-python/issues/577
rm '/usr/local/bin/2to3'

brew update
brew install \
  boost \
  cmake \
  fmt \
  gcc \
  libarchive \
  lz4 \
  mariadb-connector-c \
  msgpack-cxx \
  spdlog \
  pkg-config \
  zstd
