#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

cUsage="Usage: ${BASH_SOURCE[0]} <clp-core-build-dir>"
if [ "$#" -lt 1 ]; then
    echo "${cUsage}"
    exit
fi
build_dir="$1"

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
build_cmd=(
    docker build
    --pull
    --tag clp-core-x86-ubuntu-jammy:dev
    "$build_dir"
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
