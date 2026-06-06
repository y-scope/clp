#!/usr/bin/env bash

# Front door for local macOS tarball builds.
#
# By default this uses the Dockerized OSXCross flow so the host machine only
# needs Docker. Use --native to build directly on a Mac with local Xcode,
# Homebrew, and pipx dependencies.
#
# Usage: ./components/core/tools/packaging/macos-arm64-tarball/build.sh [OPTIONS]
#
# Common options:
#   --cores N       Parallel build jobs
#   --version VER   Package version (default: from taskfile.yaml)
#   --output DIR    Output directory for the tarball (default: ./packages)
#   --arch ARCH     Target architecture: arm64 or x86_64 (default: arm64 for
#                   OSXCross, host architecture for native builds)
#   --deployment-target VER
#                   Minimum macOS version for generated binaries (default: 14.0)
#   --clean         Remove the CLP build directory before building; also allow replacing
#                   existing real build/ and .task/ directories with build-family symlinks
#   --native        Build directly on the host Mac instead of Docker/OSXCross
#   --help          Show this help message
#
# Docker/OSXCross options:
#   --base-image IMAGE
#   --image-tag TAG, --toolchain-image TAG
#   --builder-platform PLATFORM
#   --no-build-image

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

mode="cross"
forward_args=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --native)
            mode="native"
            shift ;;
        --cross)
            mode="cross"
            shift ;;
        --help)
            sed -nE '/^# Usage:/,/^[^#]/{ /^#/s/^# ?//p; }' "$0"
            exit 0 ;;
        *)
            forward_args+=("$1")
            shift ;;
    esac
done

case "${mode}" in
    native)
        exec "${script_dir}/build-native.sh" ${forward_args[@]+"${forward_args[@]}"} ;;
    cross)
        exec "${script_dir}/build-cross.sh" ${forward_args[@]+"${forward_args[@]}"} ;;
    *)
        echo "ERROR: unknown build mode: ${mode}" >&2
        exit 1 ;;
esac
