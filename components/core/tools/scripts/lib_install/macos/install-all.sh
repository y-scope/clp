#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

brew update

# Install CMake v3.31.6 since we're using yaml-cpp that doesn't yet support CMake v4 yet
cmake_formula_path=/tmp/cmake.rb
curl --fail --show-error --location --output "${cmake_formula_path}" \
  https://raw.githubusercontent.com/Homebrew/homebrew-core/b4e46db74e74a8c1650b38b1da222284ce1ec5ce/Formula/c/cmake.rb
brew install --formula "${cmake_formula_path}"

brew install \
  boost \
  coreutils \
  fmt \
  gcc \
  go-task \
  java11 \
  libarchive \
  llvm@16 \
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
