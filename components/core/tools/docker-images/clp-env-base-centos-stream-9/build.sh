#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
component_root="${script_dir}/../../../"

docker build \
    --tag clp-core-dependencies-x86-centos-stream-9:dev \
    "$component_root" \
    --file "${script_dir}/Dockerfile"
