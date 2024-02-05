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

docker build -t clp-execution-${arch_name}-ubuntu-focal:dev ${repo_root} \
    --file ${script_dir}/Dockerfile
