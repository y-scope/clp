#!/usr/bin/env bash

# Exit on any error, use of undefined variables, or failure within a pipeline
set -euo pipefail

brew update

# Install CMake v3.31.6 as ANTLR and yaml-cpp do not yet support CMake v4+.
# See also: https://github.com/y-scope/clp/issues/795
if command -v cmake ; then
    brew uninstall --force cmake
fi
pipx install "cmake~=3.31"

# Install a version of `task` < 3.43 to avoid https://github.com/y-scope/clp/issues/872
if command -v task ; then
    brew uninstall --force task
fi
pipx install "go-task-bin>=3.40,<3.43"

# Install uv
if ! command -v uv ; then
    pipx install "uv>=0.8"
fi

brew install \
  coreutils \
  gcc \
  java11 \
  libarchive \
  llvm@16 \
  lz4 \
  mariadb-connector-c \
  xz \
  zstd

# Install pkg-config if it isn't already installed
# NOTE: We might expect that pkg-config is installed through brew, so trying to install it again
# would be harmless; however, in certain environments, like the macOS GitHub hosted runner,
# pkg-config is installed by other means, meaning a brew install would cause conflicts.
if ! command -v pkg-config ; then
    brew install pkg-config
fi

# TODO: https://github.com/y-scope/clp/issues/795
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
"${script_dir}/../check-cmake-version.sh"
