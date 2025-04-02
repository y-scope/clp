#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

# Get the installed cmake version string
cmake_version=$(cmake --version | head --lines 1 | awk '{print $3}')
cmake_major_version=$(echo "$cmake_version" | cut -d. -f1)

# Check if version is 4.0 or higher
# shellcheck disable=SC2071
if ! [[ "$cmake_version" < "4" ]]; then
  echo "CMake version $cmake_version is currently unsupported (>= 4.0)."
  exit 1
fi
