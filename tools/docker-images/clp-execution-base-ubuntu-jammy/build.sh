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

pushd "$script_dir"
if command -v git && git rev-parse --is-inside-work-tree >/dev/null ; then
    build_cmd+=(
        --label "org.opencontainers.image.revision=$(git rev-parse HEAD)"
        --label "org.opencontainers.image.source=$(git config --get remote.origin.url)"
    )
fi
popd

"${build_cmd[@]}"
