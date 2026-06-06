#!/usr/bin/env bash

# Prepares a native macOS build environment, then delegates the common
# build/package pipeline to build-package.sh.

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

# shellcheck source=defaults.sh
. "${script_dir}/defaults.sh"

args=("$@")
macos_arch="${CLP_MACOS_ARCH:-}"

for arg in "$@"; do
    if [[ "${arg}" == "--help" ]]; then
        "${script_dir}/build-package.sh" --help
        exit 0
    fi
done

while [[ $# -gt 0 ]]; do
    case "$1" in
        --arch)
            [[ -n "${2:-}" ]] || { echo "ERROR: --arch requires a value" >&2; exit 1; }
            macos_arch="$(clp_macos_normalize_arch "$2")"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done
set -- "${args[@]}"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "ERROR: native macOS packaging must run on macOS (got $(uname -s))" >&2
    echo "       Use build.sh without --native for the Dockerized OSXCross build." >&2
    exit 1
fi

host_arch="$(clp_macos_normalize_arch "$(uname -m)")"
if [[ -z "${macos_arch}" ]]; then
    macos_arch="${host_arch}"
fi
if [[ "${macos_arch}" != "${host_arch}" ]]; then
    echo "ERROR: native macOS packaging must target the host architecture." >&2
    echo "       Host: ${host_arch}; requested: ${macos_arch}" >&2
    exit 1
fi

for tool in brew cmake task otool install_name_tool codesign strip git; do
    if ! command -v "${tool}" &>/dev/null; then
        echo "ERROR: ${tool} not found on PATH" >&2
        echo "       Run components/core/tools/scripts/lib_install/macos/install-all.sh" >&2
        echo "       and components/core/tools/scripts/lib_install/pipx-packages/install-all.sh" >&2
        exit 1
    fi
done

export CLP_MACOS_ARCH="${macos_arch}"
export CLP_MACOS_BUILD_FAMILY="macos-${macos_arch}"
export CLP_MACOS_BUILD_TITLE="CLP macOS ${macos_arch} Native Tarball Build"

exec "${script_dir}/build-package.sh" "$@"
