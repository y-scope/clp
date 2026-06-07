#!/usr/bin/env bash

# Shared macOS tarball build pipeline. Environment-specific entrypoints
# prepare compilers/dependencies first, then call this script to run:
# init.sh -> task deps:core -> CMake configure/build -> package.sh.
#
# Required environment variables:
#   CLP_MACOS_BUILD_FAMILY  Cache family name, e.g. macos-arm64
#                            or macos-x86_64-cross
#
# Optional environment variables:
#   CLP_MACOS_BUILD_TITLE       Human-readable build title for logs
#   CLP_MACOS_BUILD_DETAILS     Extra preformatted config lines for logs
#   CLP_MACOS_CMAKE_ARGS        Extra CMake configure args
#   CLP_MACOS_CLEAN_EXTRA_PATHS Newline-separated extra paths to remove on --clean
#   CLP_MACOS_POST_DEPS_HOOK    Executable hook called after task deps:core
#   CLP_MACOS_POST_DEPS_LABEL   Log label for the post-deps hook
#
# Usage: ./components/core/tools/packaging/macos-arm64-tarball/build-package.sh [OPTIONS]
#
# Options:
#   --cores N       Parallel build jobs (default: number of online CPUs)
#   --version VER   Package version (default: from taskfile.yaml)
#   --output DIR    Output directory for the tarball (default: ./packages)
#   --arch ARCH     Target architecture: arm64 or x86_64
#   --deployment-target VER
#                   Minimum macOS version for generated binaries (default: 14.0)
#   --clean         Remove the CLP build directory before building; also allow replacing
#                   existing real build/ and .task/ directories with build-family symlinks
#   --help          Show this help message

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
repo_root="$(cd "${script_dir}/../../../../.." && pwd)"

# shellcheck source=defaults.sh
. "${script_dir}/defaults.sh"
# shellcheck source=../common/build-steps.sh
. "${script_dir}/../common/build-steps.sh"
# shellcheck source=../common/build-family.sh
. "${script_dir}/../common/build-family.sh"
# shellcheck source=../common/package-output.sh
. "${script_dir}/../common/package-output.sh"
# shellcheck source=../common/package-version.sh
. "${script_dir}/../common/package-version.sh"

cores="$(getconf _NPROCESSORS_ONLN 2>/dev/null || nproc 2>/dev/null || echo 4)"
version=""
output_dir="${repo_root}/packages"
deployment_target="${MACOSX_DEPLOYMENT_TARGET:-${CLP_MACOS_DEFAULT_DEPLOYMENT_TARGET}}"
macos_arch="$(clp_macos_normalize_arch "${CLP_MACOS_ARCH:-${CLP_MACOS_DEFAULT_ARCH}}")"
clean=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --cores)   [[ -n "${2:-}" ]] || { echo "ERROR: --cores requires a value" >&2; exit 1; }
                   cores="$2";       shift 2 ;;
        --version) [[ -n "${2:-}" ]] || { echo "ERROR: --version requires a value" >&2; exit 1; }
                   version="$2";     shift 2 ;;
        --output)  [[ -n "${2:-}" ]] || { echo "ERROR: --output requires a value" >&2; exit 1; }
                   output_dir="$2";  shift 2 ;;
        --arch)    [[ -n "${2:-}" ]] || { echo "ERROR: --arch requires a value" >&2; exit 1; }
                   macos_arch="$(clp_macos_normalize_arch "$2")"; shift 2 ;;
        --deployment-target)
                   [[ -n "${2:-}" ]] || { echo "ERROR: --deployment-target requires a value" >&2; exit 1; }
                   deployment_target="$2"; shift 2 ;;
        --clean)   clean=true;       shift ;;
        --help)    sed -nE '/^# Usage:/,/^[^#]/{ /^#/s/^# ?//p; }' "$0"; exit 0 ;;
        *)         echo "Unknown option: $1"; echo "Use --help for usage" >&2; exit 1 ;;
    esac
done

build_family="${CLP_MACOS_BUILD_FAMILY:-}"
if [[ -z "${build_family}" ]]; then
    echo "ERROR: CLP_MACOS_BUILD_FAMILY is required" >&2
    exit 1
fi

version="$(clp_packaging_resolve_version "${repo_root}" "${version}")"

output_dir="$(clp_packaging_resolve_output_dir "${output_dir}")"
export MACOSX_DEPLOYMENT_TARGET="${deployment_target}"
export CLP_MACOS_ARCH="${macos_arch}"

# Keep each macOS build mode isolated from Linux packaging and from each other.
build_family_dir="${repo_root}/build-${build_family}"
task_family_dir="${repo_root}/.task-${build_family}"
clp_packaging_activate_build_family "${repo_root}" "${build_family}" "${clean}"

build_dir="${repo_root}/build/core"
build_title="${CLP_MACOS_BUILD_TITLE:-CLP macOS ${macos_arch} Tarball Build}"

echo "==> ${build_title}"
echo "    Version:  ${version}"
echo "    Arch:     ${macos_arch}"
echo "    Cores:    ${cores}"
echo "    Output:   ${output_dir}"
echo "    Build:    ${build_dir}"
echo "    Min macOS: ${deployment_target}"
if [[ -n "${CLP_MACOS_BUILD_DETAILS:-}" ]]; then
    printf "%s\n" "${CLP_MACOS_BUILD_DETAILS}"
fi
echo ""

clp_packaging_remove_stale_outputs "${output_dir}" "clp-core-*-macos-${macos_arch}.tar.gz"

if [[ "${clean}" == "true" ]]; then
    echo "==> Cleaning ${build_dir}..."
    rm -rf "${build_dir}"
    if [[ -n "${CLP_MACOS_CLEAN_EXTRA_PATHS:-}" ]]; then
        while IFS= read -r clean_path; do
            [[ -n "${clean_path}" ]] || continue
            echo "==> Cleaning ${clean_path}..."
            rm -rf "${clean_path}"
        done <<< "${CLP_MACOS_CLEAN_EXTRA_PATHS}"
    fi
fi

clp_packaging_init_dependencies "${repo_root}"
clp_packaging_build_cpp_dependencies "${repo_root}" "${cores}"

if [[ -n "${CLP_MACOS_POST_DEPS_HOOK:-}" ]]; then
    echo "==> ${CLP_MACOS_POST_DEPS_LABEL:-Running post-deps hook}..."
    "${CLP_MACOS_POST_DEPS_HOOK}" "${repo_root}" "${build_dir}"
fi

echo "==> Configuring CMake..."
cmake_args=(
    -S "${repo_root}/components/core"
    -B "${build_dir}"
    -DCMAKE_BUILD_TYPE=Release
    -DCLP_USE_STATIC_LIBS=OFF
    -DCMAKE_OSX_ARCHITECTURES="${macos_arch}"
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${deployment_target}"
)
if [[ -n "${CLP_MACOS_CMAKE_ARGS:-}" ]]; then
    # Intentional word splitting for shell-style extra CMake args.
    # shellcheck disable=SC2206
    extra_cmake_args=(${CLP_MACOS_CMAKE_ARGS})
    cmake_args+=("${extra_cmake_args[@]}")
fi
cmake "${cmake_args[@]}"

echo "==> Building binaries..."
cmake --build "${build_dir}" -j "${cores}"

echo "==> Packaging..."
PKG_VERSION="${version}" \
PKG_ARCH="${macos_arch}" \
BIN_DIR="${build_dir}" \
OUTPUT_DIR="${output_dir}" \
    "${script_dir}/package.sh"

if [[ "${EUID}" -eq 0 && -n "${HOST_UID:-}" && -n "${HOST_GID:-}" ]]; then
    chown -R "${HOST_UID}:${HOST_GID}" \
        "${output_dir}" "${build_family_dir}" "${task_family_dir}" || true
fi

echo ""
echo "========================================"
echo "Build complete"
echo "========================================"
ls -lh "${output_dir}"/clp-core-*-macos-"${macos_arch}".tar.gz
echo ""
echo "Test extract:"
echo "  mkdir -p /tmp/clp-core-test && tar -xzf '${output_dir}'/clp-core-*-macos-${macos_arch}.tar.gz -C /tmp/clp-core-test"
echo "  /tmp/clp-core-test/clp-core-*/bin/clp --help"
