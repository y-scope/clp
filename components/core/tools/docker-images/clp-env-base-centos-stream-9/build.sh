#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
component_root="${script_dir}/../../../"

build_cmd=(
    docker build
    --tag clp-core-dependencies-x86-centos-stream-9:dev
    "$component_root"
    --file "${script_dir}/Dockerfile"
)

if command -v git ; then
    build_cmd+=(
        --label "org.opencontainers.image.revision=$(git rev-parse HEAD)"
        --label "org.opencontainers.image.source=$(git config --get remote.origin.url)"
    )
fi

"${build_cmd[@]}"
