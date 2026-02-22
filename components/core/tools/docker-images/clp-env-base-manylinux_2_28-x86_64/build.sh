#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
component_root="${script_dir}/../../../"

build_cmd=(
    docker buildx build
    --pull
    --platform linux/amd64
    --build-arg "ALMALINUX_MIRROR=${ALMALINUX_MIRROR:-}"
    --build-arg "QUAY_MIRROR=${QUAY_MIRROR:-quay.io}"
    --tag clp-core-dependencies-x86-manylinux_2_28:dev
    "$component_root"
    --file "${script_dir}/Dockerfile"
    --load
)

if [[ "${USE_NETWORK_HOST:-}" == "true" ]]; then
    build_cmd+=(--network=host)
fi

if command -v git >/dev/null && git -C "$script_dir" rev-parse --is-inside-work-tree >/dev/null ;
then
    build_cmd+=(
        --label "org.opencontainers.image.revision=$(git -C "$script_dir" rev-parse HEAD)"
        --label "org.opencontainers.image.source=$(git -C "$script_dir" remote get-url origin)"
    )
fi

echo "Running: ${build_cmd[*]}"
"${build_cmd[@]}"
