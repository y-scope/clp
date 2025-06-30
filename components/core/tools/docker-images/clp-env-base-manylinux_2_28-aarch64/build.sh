#!/usr/bin/env bash

set -eu
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
component_root="${script_dir}/../../../"

build_cmd=(
    docker buildx build
    --platform linux/arm64
    --tag clp-core-dependencies-aarch64-manylinux_2_28:dev
    "$component_root"
    --file "${script_dir}/Dockerfile"
    --load
)

if command -v git >/dev/null && git -C "$script_dir" rev-parse --is-inside-work-tree >/dev/null ;
then
    build_cmd+=(
        --label "org.opencontainers.image.revision=$(git -C "$script_dir" rev-parse HEAD)"
        --label "org.opencontainers.image.source=$(git -C "$script_dir" remote get-url origin)"
    )
fi

echo "Running: ${build_cmd[*]}"
"${build_cmd[@]}"
