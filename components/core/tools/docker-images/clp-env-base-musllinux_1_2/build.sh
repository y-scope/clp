#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

# Builds the CLP-core dependencies image for musllinux_1_2.
#
# By default the image is built for the host's native architecture. To cross-build
# for a different architecture (using QEMU emulation), set the PLATFORM env var,
# e.g.:
#   PLATFORM=linux/arm64 ./build.sh
# This requires binfmt/QEMU support on the host, e.g.:
#   docker run --privileged --rm tonistiigi/binfmt --install all

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
component_root="${script_dir}/../../../"
os_name="musllinux_1_2"

# Determine the target platform and the arch-specific image tag prefix.
if [[ -n "${PLATFORM:-}" ]]; then
    platform="${PLATFORM}"
else
    case "$(uname -m)" in
        x86_64|amd64) platform="linux/amd64" ;;
        aarch64|arm64) platform="linux/arm64" ;;
        *)
            echo >&2 "ERROR: Unsupported native architecture '$(uname -m)'; set PLATFORM explicitly."
            exit 1
            ;;
    esac
fi
case "${platform}" in
    linux/amd64|linux/x86_64) arch_prefix="x86" ;;
    linux/arm64|linux/aarch64) arch_prefix="aarch64" ;;
    *)
        echo >&2 "ERROR: Unsupported platform '${platform}' (expected linux/amd64 or linux/arm64)."
        exit 1
        ;;
esac

# Corporate proxy support — see corporate-proxy-host.sh for details.
source "${script_dir}/../../scripts/corporate-proxy-host.sh"
prepare_ca_cert_for_build "$component_root"
trap 'cleanup_ca_cert "$component_root"' EXIT

build_cmd=(
    docker buildx build
    --platform "${platform}"
    --tag "clp-core-dependencies-${arch_prefix}-${os_name}:dev"
    "$component_root"
    --file "${script_dir}/Dockerfile"
    --load
)

# Optional env vars:
#   HTTP_PROXY / HTTPS_PROXY / NO_PROXY / ALL_PROXY — Forwarded into the build container
#   APK_MIRROR_URL  — Override Alpine mirrors (organization-internal or regional)
#   DOCKER_NETWORK  — Override Docker network mode (auto: host for localhost proxies)
#   DOCKER_PULL=true — Force pull the latest base image from the registry
finalize_build build_cmd "$script_dir" APK_MIRROR_URL
