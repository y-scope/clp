#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

brew update
brew install \
  boost \
  cmake \
  coreutils \
  fmt \
  gcc \
  go-task \
  java11 \
  libarchive \
  lz4 \
  mariadb-connector-c \
  mongo-cxx-driver \
  msgpack-cxx \
  spdlog \
  pkg-config \
  zstd
