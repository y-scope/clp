#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
component_root="${script_dir}/../../../"

build_cmd=(
    docker build
    --tag clp-core-dependencies-x86-centos-stream-9:dev
    "$component_root"
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
