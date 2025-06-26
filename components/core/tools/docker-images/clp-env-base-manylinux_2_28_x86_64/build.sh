#!/bin/bash

set -euo pipefail

# Get the directory this script is in
script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
component_root=${script_dir}/../../../

# Build x86_64 image, will automatically use QEMU emulate if not on native platform
build_cmd=(
    docker buildx build
    --platform linux/amd64
    --tag clp-core-dependencies-manylinux_2_28_x86_64:dev
    "$component_root"
    --file "${script_dir}/Dockerfile"
    --load
)

# If in a git repo, add labels for revision and source
if command -v git >/dev/null && git -C "$script_dir" rev-parse --is-inside-work-tree >/dev/null ;
then
    build_cmd+=(
        --label "org.opencontainers.image.revision=$(git -C "$script_dir" rev-parse HEAD)"
        --label "org.opencontainers.image.source=$(git -C "$script_dir" remote get-url origin)"
    )
fi

echo "Running: ${build_cmd[*]}"

# Execute the docker buildx command
"${build_cmd[@]}"
