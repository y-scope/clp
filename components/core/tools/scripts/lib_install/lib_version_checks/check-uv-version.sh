#!/usr/bin/env bash

# Exit on any error, use of undefined variables, or failure within a pipeline
set -euo pipefail

# Get the installed uv version string
uv_version=$(uv self version --output-format json | jq --raw-output ".version")
IFS=. read -r uv_major_version uv_minor_version _ <<< "${uv_version}"

# Check version constraints
if [ "${uv_major_version}" -lt "1" ] && [ "${uv_minor_version}" -lt "8" ]; then
  echo "Error: uv version ${uv_version} is currently unsupported (< 0.8)."
  exit 1
fi
