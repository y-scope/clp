#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
component_root="${script_dir}/../../../"

# shellcheck source=components/core/tools/scripts/docker-image-build.sh
source "${script_dir}/../../scripts/docker-image-build.sh"
parse_build_args "$@"

build_cmd=(
    docker buildx build
    --tag clp-core-dependencies-x86-ubuntu-jammy:dev
    --file "${script_dir}/Dockerfile"
    --load
    "$component_root"
)

# Optional env vars:
#   HTTP_PROXY / HTTPS_PROXY / NO_PROXY / ALL_PROXY — Forwarded into the build container
#   APT_MIRROR_URL  — Override Ubuntu mirrors (organization-internal or regional)
#   DOCKER_NETWORK  — Override Docker network mode (auto: host for localhost proxies)
#   DOCKER_PULL=false — Skip pulling the latest base image from the registry
run_image_build build_cmd "$script_dir" APT_MIRROR_URL
