#!/bin/bash

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
component_root=${script_dir}/../../../

build_cmd=(
    docker build
    --tag clp-core-dependencies-x86-ubuntu-jammy:dev
    "$component_root"
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
