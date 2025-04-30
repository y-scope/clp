#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
repo_root=${script_dir}/../../../

arch=$(uname -m)

if [ "$arch" = "x86_64" ]; then
    arch_name="x86"
elif [ "$arch" = "aarch64" ]; then
    arch_name="arm64"
else
    echo "Error: Unsupported architecture - $arch"
    exit 1
fi

build_cmd=(
    docker build
    --tag clp-execution-${arch_name}-ubuntu-jammy:dev
    "$repo_root"
    --file "${script_dir}/Dockerfile"
)

if command -v git >/dev/null && git -C "$script_dir" rev-parse --is-inside-work-tree >/dev/null ;
then
    build_cmd+=(
        --label "org.opencontainers.image.revision=$(git -C "$script_dir" rev-parse HEAD)"
        --label "org.opencontainers.image.source=$(git -C "$script_dir" remote get-url origin)"
    )
fi

"${build_cmd[@]}"
