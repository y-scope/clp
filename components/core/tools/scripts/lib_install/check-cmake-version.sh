#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

# Get the installed cmake version string
cmake_version=$(cmake -E capabilities | jq --raw-output ".version.string")
cmake_major_version=$(cmake -E capabilities | jq --raw-output ".version.major")

# Check if version is 4.0 or higher
if [[ "$cmake_major_version" -ge "4" ]]; then
  echo "CMake version $cmake_version is currently unsupported (>= 4.0)."
  exit 1
fi
