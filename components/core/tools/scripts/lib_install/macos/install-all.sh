#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

brew update

formula_dir="$(mktemp -d -t "clp-dep-formulas")"

# Install CMake v3.31.6 as ANTLR and yaml-cpp do not yet support CMake v4+.
# See also: https://github.com/y-scope/clp/issues/795
cmake_formula_path="${formula_dir}/cmake.rb"
curl \
  --fail \
  --location \
  --output "$cmake_formula_path" \
  --show-error \
  https://raw.githubusercontent.com/Homebrew/homebrew-core/b4e46db74e74a8c1650b38b1da222284ce1ec5ce\
/Formula/c/cmake.rb
brew install --formula "$cmake_formula_path"

# Install a version of `task` < 3.43 to avoid https://github.com/y-scope/clp/issues/872
task_formula_path="${formula_dir}/go-task.rb"
curl \
  --fail \
  --location \
  --output "$task_formula_path" \
  --show-error \
  https://raw.githubusercontent.com/Homebrew/homebrew-core/356f8408263b6a06e8f5f83cad574773d8054e1c\
/Formula/g/go-task.rb
brew install --formula "$task_formula_path"

rm -rf "$formula_dir"

brew install \
  boost \
  coreutils \
  gcc \
  java11 \
  libarchive \
  llvm@16 \
  lz4 \
  mariadb-connector-c \
  msgpack-cxx \
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
