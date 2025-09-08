#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

cmake_version=$(cmake -E capabilities | jq --raw-output ".version.string")
cmake_major_version=$(cmake -E capabilities | jq --raw-output ".version.major")
cmake_minor_version=$(cmake -E capabilities | jq --raw-output ".version.minor")

# ystdlib requires CMake v3.23; ANTLR and yaml-cpp do not yet support CMake v4+
# (see https://github.com/y-scope/clp/issues/795).
if (( "${cmake_major_version}" < 3 )) || \
   (( "${cmake_major_version}" == 3 && "${cmake_minor_version}" < 23 )) || \
   (( "${cmake_major_version}" >= 4 )); then
  echo "Error: CMake version ${cmake_version} is unsupported (require 3.23 ≤ version < 4.0)."
  exit 1
fi
