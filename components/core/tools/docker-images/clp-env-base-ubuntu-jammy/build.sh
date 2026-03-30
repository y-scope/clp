#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
component_root=${script_dir}/../../../

# Corporate proxy support — see proxy-lib.sh for details.
source "${script_dir}/../proxy-lib.sh"
prepare_ca_cert_for_build "$component_root"
trap 'cleanup_ca_cert "$component_root"' EXIT

build_cmd=(
    docker build
    --tag clp-core-dependencies-x86-ubuntu-jammy:dev
    "$component_root"
    --file "${script_dir}/Dockerfile"
)

# Optional env vars:
#   HTTP_PROXY / HTTPS_PROXY / NO_PROXY / ALL_PROXY — Forwarded into the build container
#   APT_MIRROR_URL  — Override Ubuntu mirrors (organization-internal or regional)
#   DOCKER_NETWORK  — Override Docker network mode (auto: host for localhost proxies)
#   DOCKER_PULL=true — Force pull the latest base image from the registry
finalize_build build_cmd "$script_dir" APT_MIRROR_URL
