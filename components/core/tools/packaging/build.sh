#!/usr/bin/env bash

# Builds one CLP core package for one target OS, architecture, format, and libc.
#
# Supported targets:
#   linux + deb      + glibc
#   linux + rpm      + glibc
#   linux + apk      + musl
#   linux + tarball  + glibc|musl
#   macos + tarball
#
# Linux architecture builds use Docker platform selection. A non-host
# architecture runs through Docker/QEMU rather than a hand-rolled cross
# toolchain. macOS Docker builds normally run a host-platform OSXCross image;
# --arch selects the Mach-O target architecture that image cross-compiles.
#
# Usage: ./components/core/tools/packaging/build.sh [OPTIONS]
#
# Required:
#   --format FMT    Package format: deb, rpm, apk, or tarball
#
# Common options:
#   --target-os OS  Target OS: linux or macos (default: host OS)
#   --arch ARCH     Target architecture: host, x86_64, or arm64 (default: host)
#   --libc LIBC     Linux libc: glibc or musl
#                   Defaults to glibc for deb/rpm/linux tarball, musl for apk.
#                   Invalid for macOS.
#   --cores N       Parallel build jobs (default: nproc/getconf)
#   --version VER   Package version (default: from taskfile.yaml)
#   --output DIR    Output root directory for packages (default: ./packages).
#                   Packages are written under DIR/<arch>/.
#   --clean         Remove this target's build artifacts before building; also allow
#                   replacing existing real build/ and .task/ directories with
#                   build-family symlinks
#   --help          Show this help message
#
# macOS-only options passed through to macos-arm64-tarball/build.sh:
#   --native
#   --deployment-target VER
#   --base-image IMAGE
#   --image-tag TAG, --toolchain-image TAG
#   --builder-platform PLATFORM  Advanced override; default is Docker host platform
#   --no-build-image
#
# Packages are grouped under OUTPUT_DIR/<arch>/, where <arch> is the normalized
# --arch value (x86_64 or arm64), and each arch directory remains flat.

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

# shellcheck source=common/build-family.sh
. "${script_dir}/common/build-family.sh"
# shellcheck source=common/package-output.sh
. "${script_dir}/common/package-output.sh"
# shellcheck source=common/package-metadata.sh
. "${script_dir}/common/package-metadata.sh"
# shellcheck source=common/package-version.sh
. "${script_dir}/common/package-version.sh"

target_os=""
format=""
target_arch="host"
target_libc=""
cores="$(getconf _NPROCESSORS_ONLN 2>/dev/null || nproc 2>/dev/null || echo 4)"
version=""
output_dir="${repo_root}/packages"
clean=false

macos_native=false
macos_deployment_target=""
macos_base_image=""
macos_image_tag=""
macos_builder_platform=""
macos_build_image=true

die() {
    echo "ERROR: $*" >&2
    exit 1
}

host_os() {
    case "$(uname -s)" in
        Linux)  printf 'linux\n' ;;
        Darwin) printf 'macos\n' ;;
        *)      return 1 ;;
    esac
}

normalize_os() {
    case "$1" in
        linux)       printf 'linux\n' ;;
        macos|darwin) printf 'macos\n' ;;
        all)         die "--target-os all is not supported; run one target OS per invocation" ;;
        *)           die "Unsupported target OS: $1 (use linux or macos)" ;;
    esac
}

normalize_host_arch() {
    case "$(uname -m)" in
        x86_64|amd64)       printf 'x86_64\n' ;;
        aarch64|arm64|arm)  printf 'arm64\n' ;;
        *)                  die "Unsupported host architecture: $(uname -m)" ;;
    esac
}

normalize_arch() {
    case "$1" in
        host)               normalize_host_arch ;;
        x86|x86_64|amd64)   printf 'x86_64\n' ;;
        arm|arm64|aarch64)  printf 'arm64\n' ;;
        all)                die "--arch all is not supported; run one architecture per invocation" ;;
        *)                  die "Unsupported architecture: $1 (use host, x86_64, or arm64)" ;;
    esac
}

normalize_format() {
    case "$1" in
        deb|rpm|apk|tarball)
            printf '%s\n' "$1"
            ;;
        all)
            die "--format all is not supported; run one package format per invocation"
            ;;
        tarball-musl)
            die "Use --format tarball --libc musl instead of --format tarball-musl"
            ;;
        *,*)
            die "--format accepts one value per invocation, got: $1"
            ;;
        *)
            die "Unsupported format: $1 (use deb, rpm, apk, or tarball)"
            ;;
    esac
}

normalize_libc() {
    case "$1" in
        glibc|musl) printf '%s\n' "$1" ;;
        all)        die "--libc all is not supported; run one libc per invocation" ;;
        *,*)        die "--libc accepts one value per invocation, got: $1" ;;
        *)          die "Unsupported libc: $1 (use glibc or musl)" ;;
    esac
}

linux_job_artifact_pattern() {
    local cur_format="$1"
    local pkg_arch="$2"
    local pkg_libc="$3"

    case "${cur_format}" in
        deb)     printf '%s\n' "${CLP_CORE_PACKAGE_NAME}_*_${pkg_arch}.deb" ;;
        rpm)     printf '%s\n' "${CLP_CORE_PACKAGE_NAME}-*.${pkg_arch}.rpm" ;;
        apk)     printf '%s\n' "${CLP_CORE_PACKAGE_NAME}-*.apk" ;;
        tarball) printf '%s\n' "${CLP_CORE_PACKAGE_NAME}-*-linux-${pkg_libc}-${pkg_arch}.tar.gz" ;;
        *)       die "Unsupported Linux format: ${cur_format}" ;;
    esac
}

print_help() {
    sed -nE '/^# Usage:/,/^[^#]/{ /^#/s/^# ?//p; }' "$0"
}

# --- Argument parsing --------------------------------------------------------

while [[ $# -gt 0 ]]; do
    case $1 in
        --target-os|--os)
            [[ -n "${2:-}" ]] || die "$1 requires a value"
            target_os="$2"; shift 2 ;;
        --format)
            [[ -n "${2:-}" ]] || die "--format requires a value"
            format="$2"; shift 2 ;;
        --arch)
            [[ -n "${2:-}" ]] || die "--arch requires a value"
            target_arch="$2"; shift 2 ;;
        --libc)
            [[ -n "${2:-}" ]] || die "--libc requires a value"
            target_libc="$2"; shift 2 ;;
        --cores)
            [[ -n "${2:-}" ]] || die "--cores requires a value"
            cores="$2"; shift 2 ;;
        --version)
            [[ -n "${2:-}" ]] || die "--version requires a value"
            version="$2"; shift 2 ;;
        --output)
            [[ -n "${2:-}" ]] || die "--output requires a value"
            output_dir="$2"; shift 2 ;;
        --clean)
            clean=true; shift ;;
        --native)
            macos_native=true; shift ;;
        --deployment-target)
            [[ -n "${2:-}" ]] || die "--deployment-target requires a value"
            macos_deployment_target="$2"; shift 2 ;;
        --base-image)
            [[ -n "${2:-}" ]] || die "--base-image requires a value"
            macos_base_image="$2"; shift 2 ;;
        --image-tag|--toolchain-image)
            [[ -n "${2:-}" ]] || die "$1 requires a value"
            macos_image_tag="$2"; shift 2 ;;
        --builder-platform)
            [[ -n "${2:-}" ]] || die "--builder-platform requires a value"
            macos_builder_platform="$2"; shift 2 ;;
        --no-build-image)
            macos_build_image=false; shift ;;
        --help)
            print_help
            exit 0 ;;
        *)
            echo "Unknown option: $1" >&2
            echo "Use --help for usage" >&2
            exit 1 ;;
    esac
done

# --- Resolve and validate the requested single job ---------------------------

if [[ -z "${format}" ]]; then
    die "--format is required (use deb, rpm, apk, or tarball)"
fi
format="$(normalize_format "${format}")"

if [[ -z "${target_os}" ]]; then
    if ! target_os="$(host_os)"; then
        die "Unsupported host OS: $(uname -s); specify --target-os linux or --target-os macos"
    fi
else
    target_os="$(normalize_os "${target_os}")"
fi

target_arch="$(normalize_arch "${target_arch}")"
if [[ -n "${target_libc}" ]]; then
    target_libc="$(normalize_libc "${target_libc}")"
fi

# --- macOS -------------------------------------------------------------------

if [[ "${target_os}" == "macos" ]]; then
    [[ "${format}" == "tarball" ]] || die "macOS packaging only supports --format tarball"
    [[ -z "${target_libc}" ]] || die "--libc is not valid for macOS builds"

    version="$(clp_packaging_resolve_version "${repo_root}" "${version}")"
    output_dir="$(clp_packaging_resolve_output_dir "${output_dir}")"
    output_package_dir="${output_dir}/${target_arch}"

    macos_cmd=("${script_dir}/macos-arm64-tarball/build.sh")
    macos_cmd+=(--arch "${target_arch}")
    macos_cmd+=(--cores "${cores}")
    macos_cmd+=(--version "${version}")
    macos_cmd+=(--output "${output_package_dir}")
    "${clean}" && macos_cmd+=(--clean)
    "${macos_native}" && macos_cmd+=(--native)
    [[ -n "${macos_deployment_target}" ]] && macos_cmd+=(--deployment-target "${macos_deployment_target}")
    [[ -n "${macos_base_image}" ]] && macos_cmd+=(--base-image "${macos_base_image}")
    [[ -n "${macos_image_tag}" ]] && macos_cmd+=(--image-tag "${macos_image_tag}")
    [[ -n "${macos_builder_platform}" ]] && macos_cmd+=(--builder-platform "${macos_builder_platform}")
    "${macos_build_image}" || macos_cmd+=(--no-build-image)

    echo "==> CLP Package Build"
    echo "    Target OS: macos"
    echo "    Format:    tarball"
    echo "    Arch:      ${target_arch}"
    echo "    Version:   ${version}"
    echo "    Cores:     ${cores}"
    echo "    Output:    ${output_package_dir}"
    echo ""

    exec "${macos_cmd[@]}"
fi

# --- Linux -------------------------------------------------------------------

[[ "${target_os}" == "linux" ]] || die "Unsupported target OS: ${target_os}"

if [[ "${macos_native}" == "true" || -n "${macos_deployment_target}" || -n "${macos_base_image}" \
        || -n "${macos_image_tag}" || -n "${macos_builder_platform}" || "${macos_build_image}" == "false" ]]; then
    die "macOS-only options cannot be used with --target-os linux"
fi

if ! command -v docker &>/dev/null; then
    die "docker is required for Linux package builds"
fi

case "${format}" in
    deb|rpm)
        if [[ -z "${target_libc}" ]]; then
            target_libc="glibc"
        fi
        [[ "${target_libc}" == "glibc" ]] || die "--format ${format} requires --libc glibc"
        ;;
    apk)
        if [[ -z "${target_libc}" ]]; then
            target_libc="musl"
        fi
        [[ "${target_libc}" == "musl" ]] || die "--format apk requires --libc musl"
        ;;
    tarball)
        if [[ -z "${target_libc}" ]]; then
            target_libc="glibc"
        fi
        ;;
esac

case "${target_arch}" in
    x86_64)
        docker_suffix="x86_64"
        docker_platform="linux/amd64"
        image_arch_tag="x86"
        deb_arch="amd64"
        linux_pkg_arch="x86_64"
        ;;
    arm64)
        docker_suffix="aarch64"
        docker_platform="linux/arm64"
        image_arch_tag="aarch64"
        deb_arch="arm64"
        linux_pkg_arch="aarch64"
        ;;
    *)
        die "Unsupported Linux architecture: ${target_arch}"
        ;;
esac

case "${format}:${target_libc}" in
    deb:glibc)
        format_dir="${script_dir}/universal-deb"
        base_image_family="manylinux_2_28"
        dockerfile_dir="${script_dir}/universal-deb"
        builder_image_prefix="clp-manylinux-pkg-builder"
        pkg_libc=""
        ;;
    rpm:glibc)
        format_dir="${script_dir}/universal-rpm"
        base_image_family="manylinux_2_28"
        dockerfile_dir="${script_dir}/universal-deb"
        builder_image_prefix="clp-manylinux-pkg-builder"
        pkg_libc=""
        ;;
    tarball:glibc)
        format_dir="${script_dir}/linux-tarball"
        base_image_family="manylinux_2_28"
        dockerfile_dir="${script_dir}/universal-deb"
        builder_image_prefix="clp-manylinux-pkg-builder"
        pkg_libc="glibc"
        ;;
    apk:musl)
        format_dir="${script_dir}/alpine-apk"
        base_image_family="musllinux_1_2"
        dockerfile_dir="${script_dir}/alpine-apk"
        builder_image_prefix="clp-apk-builder"
        pkg_libc=""
        ;;
    tarball:musl)
        format_dir="${script_dir}/linux-tarball"
        base_image_family="musllinux_1_2"
        dockerfile_dir="${script_dir}/alpine-apk"
        builder_image_prefix="clp-apk-builder"
        pkg_libc="musl"
        ;;
    *)
        die "Unsupported Linux package combination: format=${format}, libc=${target_libc}"
        ;;
esac

[[ -f "${format_dir}/package.sh" ]] || die "Package script not found: ${format_dir}/package.sh"

if [[ "${format}" == "deb" ]]; then
    pkg_arch="${deb_arch}"
else
    pkg_arch="${linux_pkg_arch}"
fi

version="$(clp_packaging_resolve_version "${repo_root}" "${version}")"
output_dir="$(clp_packaging_resolve_output_dir "${output_dir}")"

base_image_tag="clp-core-dependencies-${image_arch_tag}-${base_image_family}:dev"
builder_image="${builder_image_prefix}-${docker_suffix}:dev"
build_family="${base_image_family}-${docker_suffix}"
package_pattern="$(linux_job_artifact_pattern "${format}" "${pkg_arch}" "${target_libc}")"
build_package_dir="${repo_root}/build"
output_package_dir="${output_dir}/${target_arch}"

echo "==> CLP Package Build"
echo "    Target OS: linux"
echo "    Format:    ${format}"
echo "    Libc:      ${target_libc}"
echo "    Arch:      ${target_arch}"
echo "    Package arch: ${pkg_arch}"
echo "    Version:   ${version}"
echo "    Cores:     ${cores}"
echo "    Output:    ${output_dir}"
echo "    Docker platform: ${docker_platform}"
echo "    Build family: ${build_family}"
echo ""

if [[ "${clean}" == "true" ]]; then
    echo "==> Cleaning build artifacts for ${build_family}..."
    rm -rf "${repo_root}/build-${build_family}" "${repo_root}/.task-${build_family}"
fi

clp_packaging_activate_build_family "${repo_root}" "${build_family}" "${clean}"

mkdir -p "${output_package_dir}" "${build_package_dir}"
clp_packaging_remove_stale_outputs "${output_package_dir}" "${package_pattern}"
clp_packaging_remove_stale_outputs "${build_package_dir}" "${package_pattern}"

if ! docker image inspect "${base_image_tag}" &>/dev/null; then
    echo "==> Building base image ${base_image_tag}..."
    bash "${repo_root}/components/core/tools/docker-images/clp-env-base-${base_image_family}-${docker_suffix}/build.sh"
fi

if ! docker image inspect "${builder_image}" &>/dev/null; then
    echo "==> Building builder image ${builder_image}..."
    docker build \
        --platform "${docker_platform}" \
        --build-arg "BASE_IMAGE=${base_image_tag}" \
        --tag "${builder_image}" \
        "${dockerfile_dir}" \
        --file "${dockerfile_dir}/Dockerfile"
fi

echo "==> Starting build and packaging..."
# Run as root inside the container so package tools produce root-owned package
# contents and Alpine abuild can run without fakeroot.
docker run --rm \
    --platform "${docker_platform}" \
    -v "${repo_root}:/clp" \
    -w /clp \
    -e "CORES=${cores}" \
    -e "PKG_VERSION=${version}" \
    -e "PKG_ARCH=${pkg_arch}" \
    -e "PKG_LIBC=${pkg_libc}" \
    -e "HOST_UID=$(id -u)" \
    -e "HOST_GID=$(id -g)" \
    -e "CFLAGS=-U_FORTIFY_SOURCE" \
    -e "CXXFLAGS=-U_FORTIFY_SOURCE" \
    -e "FORMAT_DIR=${format_dir##*/}" \
    "${builder_image}" \
    bash /clp/components/core/tools/packaging/common/build-in-container.sh

echo "==> Copying package to ${output_package_dir}..."
if [[ -z "$(find -L "${build_package_dir}" -maxdepth 1 -name "${package_pattern}" -print -quit)" ]]; then
    die "No ${format} package found in ${build_package_dir} matching ${package_pattern}"
fi
find -L "${build_package_dir}" -maxdepth 1 -name "${package_pattern}" \
    -exec cp {} "${output_package_dir}/" \;

echo ""
echo "========================================"
echo "Build complete"
echo "========================================"
find "${output_package_dir}" -maxdepth 1 -name "${package_pattern}" -exec ls -lh {} + 2>/dev/null || true
echo ""

case "${format}" in
    deb)
        echo "Test deb:"
        echo "  docker run --rm -v '${output_package_dir}':/pkgs debian:bookworm bash -c \\"
        echo "    'dpkg -i /pkgs/${CLP_CORE_PACKAGE_NAME}_*_${pkg_arch}.deb && clp-s --help'"
        ;;
    rpm)
        echo "Test rpm:"
        echo "  docker run --rm -v '${output_package_dir}':/pkgs almalinux:9 bash -c \\"
        echo "    'rpm -i /pkgs/${CLP_CORE_PACKAGE_NAME}-*.${pkg_arch}.rpm && clp-s --help'"
        ;;
    apk)
        echo "Test apk:"
        echo "  docker run --rm -v '${output_package_dir}':/pkgs alpine:3.20 sh -c \\"
        echo "    'apk add --allow-untrusted /pkgs/${CLP_CORE_PACKAGE_NAME}-*.apk && clp-s --help'"
        ;;
    tarball)
        if [[ "${target_libc}" == "musl" ]]; then
            echo "Test musl tarball:"
            echo "  docker run --rm -v '${output_package_dir}':/pkgs alpine:3.20 sh -c \\"
            echo "    'tmpdir=\$(mktemp -d) && tar -xzf /pkgs/${CLP_CORE_PACKAGE_NAME}-*-linux-musl-${pkg_arch}.tar.gz -C \"\${tmpdir}\" && \"\${tmpdir}\"/${CLP_CORE_PACKAGE_NAME}-*/bin/clp-s --help'"
        else
            echo "Test tarball:"
            echo "  tmpdir=\$(mktemp -d) && tar -xzf '${output_package_dir}'/${CLP_CORE_PACKAGE_NAME}-*-linux-glibc-${pkg_arch}.tar.gz -C \"\${tmpdir}\""
            echo "  \"\${tmpdir}\"/${CLP_CORE_PACKAGE_NAME}-*/bin/clp-s --help"
        fi
        ;;
esac
