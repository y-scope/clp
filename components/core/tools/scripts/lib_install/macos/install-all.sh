#!/usr/bin/env bash

# Exit on any error, use of undefined variables, or failure within a pipeline
set -euo pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

brew update
brew install \
  coreutils \
  gcc \
  java11 \
  libarchive \
  llvm@16 \
  lz4 \
  mariadb-connector-c \
  pipx \
  xz \
  zstd

# Install pkg-config if it isn't already installed
# NOTE: We might expect that pkg-config is installed through brew, so trying to install it again
# would be harmless; however, in certain environments, like the macOS GitHub hosted runner,
# pkg-config is installed by other means, meaning a brew install would cause conflicts.
if ! command -v pkg-config >/dev/null 2>&1; then
    brew install pkg-config
fi

# Install CMake v3.31.6 as ANTLR and yaml-cpp do not yet support CMake v4+.
# See also: https://github.com/y-scope/clp/issues/795
if command -v cmake >/dev/null 2>&1; then
    brew uninstall --force cmake
fi
pipx install "cmake>=3.23,<3.24"

# Install a version of `task` < 3.43 to avoid https://github.com/y-scope/clp/issues/872
if command -v task >/dev/null 2>&1; then
    brew uninstall --force task
fi
pipx install "go-task-bin>=3.40,<3.43"

# Install uv
if ! command -v uv >/dev/null 2>&1; then
    pipx install "uv>=0.8"
fi

"${script_dir}/../lib_version_checks/check-build-tool-versions.sh"
