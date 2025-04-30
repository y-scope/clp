#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

cUsage="Usage: ${BASH_SOURCE[0]} <clp-core-build-dir>"
if [ "$#" -lt 1 ]; then
    echo "${cUsage}"
    exit
fi
build_dir="$1"

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
build_cmd=(
    docker build
    --tag clp-core-x86-ubuntu-jammy:dev
    "$build_dir"
    --file "${script_dir}/Dockerfile"
)

if command -v git ; then
    build_cmd+=(
        --label "org.opencontainers.image.revision=$(git rev-parse HEAD)"
        --label "org.opencontainers.image.source=$(git config --get remote.origin.url)"
    )
fi

"${build_cmd[@]}"
