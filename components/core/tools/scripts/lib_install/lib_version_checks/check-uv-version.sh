#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

uv_version=$(uv self version --output-format json | jq --raw-output ".version")
IFS=. read -r uv_major_version uv_minor_version _ <<< "${uv_version}"

if (( "${uv_major_version}" == 0 && "${uv_minor_version}" < 8 )); then
  echo "Error: uv version ${uv_version} is unsupported (require version ≥ 0.8)."
  exit 1
fi
