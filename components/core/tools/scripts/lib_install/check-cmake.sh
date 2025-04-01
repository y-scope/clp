#!/usr/bin/env bash

# Exit on error
set -e

# Get the installed cmake version string
cmake_version=$(cmake --version | head -n1 | awk '{print $3}')
cmake_major_version=$(echo "$cmake_version" | cut -d. -f1)

# Check if version is 4.0 or higher
if [[ "$cmake_major_version" -ge "4" ]]; then
  echo "CMake version $cmake_version is not supported (>= 4.0)."
  exit 1
fi
