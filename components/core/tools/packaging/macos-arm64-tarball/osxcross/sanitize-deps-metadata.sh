#!/usr/bin/env bash

# Removes Linux-only link metadata that some cross-built dependency config files
# can record while probing the Linux build host.

set -o errexit
set -o nounset
set -o pipefail

repo_root="${1:?repo root required}"
deps_dir="${repo_root}/build/deps/cpp"

if [[ ! -d "${deps_dir}" ]]; then
    exit 0
fi

find "${deps_dir}" -type f \( -name '*.cmake' -o -name '*.pc' \) -exec sed -i \
    -e 's#\$<LINK_ONLY:\$<\$<BOOL:/usr/lib64/librt\.so>:-lrt>>##g' \
    -e 's#;*/usr/lib64/librt\.so##g' \
    -e 's# /usr/lib64/librt\.so##g' \
    -e 's# -L/usr/lib64 -lrt##g' \
    -e 's# -L/usr/lib64##g' \
    -e 's# -lrt##g' \
    {} +
