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
  xz \
  zstd

# Install pkg-config if it isn't already installed
# NOTE: We might expect that pkg-config is installed through brew, so trying to install it again
# would be harmless; however, in certain environments, like the macOS GitHub hosted runner,
# pkg-config is installed by other means, meaning a brew install would cause conflicts.
if ! command -v pkg-config ; then
    brew install pkg-config
fi
