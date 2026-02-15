#!/usr/bin/env bash

# Builds CLP core binaries and packages them into universal packages.
#
# Supported formats:
#   deb  — Debian/Ubuntu package (built on manylinux_2_28, glibc >= 2.28)
#   rpm  — RHEL/Fedora package (built on manylinux_2_28, glibc >= 2.28)
#   apk  — Alpine package (built on musllinux_1_2, musl libc)
#
# The packages are "universal" — binaries are built on broad-compatibility base
# images and bundled with their non-system shared library dependencies via
# patchelf, so they work on any supported distribution without extra installs.
#
# Prerequisites: Docker (with buildx for cross-architecture builds)
#
# Usage: ./components/core/tools/packaging/build.sh [OPTIONS]
#
# Options:
#   --format FMT    Package format: deb, rpm, apk, or all (default: all)
#   --arch ARCH     Target architecture: aarch64, x86_64, or all (default: host)
#   --cores N       Parallel build jobs (default: nproc)
#   --version VER   Package version (default: from taskfile.yaml)
#   --output DIR    Output directory for packages (default: ./packages)
#   --clean         Remove build artifacts before building
#   --help          Show this help message

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
repo_root="$(cd "${script_dir}/../../../.." && pwd)"

# Defaults
format="all"
cores="$(nproc 2>/dev/null || echo 4)"
version=""
output_dir="${repo_root}/packages"
target_arches=""
clean=false

# --- Argument parsing --------------------------------------------------------

while [[ $# -gt 0 ]]; do
    case $1 in
        --format)  [[ -n "${2:-}" ]] || { echo "ERROR: --format requires a value" >&2; exit 1; }
                   format="$2";        shift 2 ;;
        --arch)    [[ -n "${2:-}" ]] || { echo "ERROR: --arch requires a value" >&2; exit 1; }
                   target_arches="$2"; shift 2 ;;
        --cores)   [[ -n "${2:-}" ]] || { echo "ERROR: --cores requires a value" >&2; exit 1; }
                   cores="$2";         shift 2 ;;
        --version) [[ -n "${2:-}" ]] || { echo "ERROR: --version requires a value" >&2; exit 1; }
                   version="$2";       shift 2 ;;
        --output)  [[ -n "${2:-}" ]] || { echo "ERROR: --output requires a value" >&2; exit 1; }
                   output_dir="$2";    shift 2 ;;
        --clean)   clean=true;         shift ;;
        --help)    sed -n '/^# Usage:/,/^[^#]/{ /^#/s/^# \?//p; }' "$0"; exit 0 ;;
        *)         echo "Unknown option: $1"; echo "Use --help for usage"; exit 1 ;;
    esac
done

# --- Validate prerequisites --------------------------------------------------

if ! command -v docker &>/dev/null; then
    echo "ERROR: docker is required" >&2
    exit 1
fi

# --- Resolve defaults --------------------------------------------------------

valid_formats="deb rpm apk"
[[ "${format}" == "all" ]] && format="deb,rpm,apk"
IFS=',' read -ra format_list <<< "${format}"

# Validate formats early (before any side effects like stale-package cleanup)
for _fmt in "${format_list[@]}"; do
    _fmt=$(echo "${_fmt}" | xargs)
    if [[ ! " ${valid_formats} " =~ " ${_fmt} " ]]; then
        echo "ERROR: Unsupported format: ${_fmt} (use deb, rpm, apk, or all)" >&2
        exit 1
    fi
done

output_dir="$(mkdir -p "${output_dir}" && cd "${output_dir}" && pwd)"

if [[ -z "${target_arches}" ]]; then
    case "$(uname -m)" in
        x86_64)        target_arches="x86_64" ;;
        aarch64|arm64) target_arches="aarch64" ;;
        *)             echo "ERROR: Unsupported host architecture: $(uname -m)" >&2; exit 1 ;;
    esac
fi
[[ "${target_arches}" == "all" ]] && target_arches="aarch64,x86_64"
IFS=',' read -ra arch_list <<< "${target_arches}"

if [[ -z "${version}" ]]; then
    version=$(grep 'G_PACKAGE_VERSION:' "${repo_root}/taskfile.yaml" \
        | head -1 \
        | sed 's/.*"\(.*\)".*/\1/')
    if [[ -z "${version}" ]]; then
        echo "ERROR: Could not extract version from taskfile.yaml and --version not provided" >&2
        exit 1
    fi
fi

# If the version has a pre-release suffix (anything after a hyphen, e.g., "-dev",
# "-beta", "-rc1"), replace it with a snapshot identifier for reproducibility.
# Any existing suffix is replaced, so passing --version "0.9.1-foo" will still
# regenerate the snapshot from the current HEAD commit.
# E.g., "0.9.1-dev" -> "0.9.1-20260214.5f1d7ca". Each package format then maps
# this to its own convention:
#   deb: 0.9.1~20260214.5f1d7ca-1  (~ pre-release, -1 debian revision)
#   rpm: 0.9.1~20260214.5f1d7ca    (~ pre-release, Release: 1 in spec)
#   apk: 0.9.1_git20260214         (_git suffix, hash in pkgdesc — apk is digits-only)
if [[ "${version}" == *-* ]]; then
    git_date=$(git -C "${repo_root}" log -1 --format=%cd --date=format:%Y%m%d)
    git_hash=$(git -C "${repo_root}" log -1 --format=%h)
    version="${version%%-*}-${git_date}.${git_hash}"
fi

# --- Print configuration ----------------------------------------------------

echo "==> CLP Universal Package Build"
echo "    Formats:  ${format_list[*]}"
echo "    Version:  ${version}"
echo "    Cores:    ${cores}"
echo "    Arches:   ${arch_list[*]}"
echo "    Output:   ${output_dir}"
echo ""

# Remove stale packages from the output directory (only for formats being built)
for _fmt in "${format_list[@]}"; do
    _fmt=$(echo "${_fmt}" | xargs)
    rm -f "${output_dir}"/clp-core*."${_fmt}"
done

# --- Helper: point build/ and .task/ at the right image family ---------------
#
# glibc and musl binaries are incompatible, so each image family gets its own
# build directory (build-manylinux_2_28/, build-musllinux_1_2/, etc.). The
# task runner expects build/ and .task/, so we symlink them to the active
# family's directories.

activate_build_family() {
    local target_family="$1"

    # Create family-specific directories if they don't exist
    mkdir -p "${repo_root}/build-${target_family}" "${repo_root}/.task-${target_family}"

    # Remove any real directory at the symlink target (ln -sfn cannot atomically
    # replace a real directory — it would create the symlink inside it instead).
    for dir_name in build .task; do
        local target="${repo_root}/${dir_name}"
        if [[ -d "${target}" && ! -L "${target}" ]]; then
            rm -rf "${target}"
        fi
    done

    # Point build/ and .task/ at the target family
    ln --symbolic --force --no-dereference "build-${target_family}" "${repo_root}/build"
    ln --symbolic --force --no-dereference ".task-${target_family}" "${repo_root}/.task"
}

# --- Build for each format and architecture ----------------------------------

for cur_format in "${format_list[@]}"; do
    cur_format=$(echo "${cur_format}" | xargs)

    # Resolve format-specific settings
    case "${cur_format}" in
        deb)
            format_dir="${script_dir}/universal-deb"
            base_image_family="manylinux_2_28"
            # deb and rpm share the same builder image (same Dockerfile installs
            # both dpkg and rpm-build) so Docker layer caching is shared.
            dockerfile_dir="${script_dir}/universal-deb"
            builder_image_prefix="clp-manylinux-pkg-builder"
            ;;
        rpm)
            format_dir="${script_dir}/universal-rpm"
            base_image_family="manylinux_2_28"
            # Shares the deb Dockerfile (installs both dpkg and rpm-build)
            dockerfile_dir="${script_dir}/universal-deb"
            builder_image_prefix="clp-manylinux-pkg-builder"
            ;;
        apk)
            format_dir="${script_dir}/alpine-apk"
            base_image_family="musllinux_1_2"
            dockerfile_dir="${script_dir}/alpine-apk"
            builder_image_prefix="clp-apk-builder"
            ;;
        *)
            echo "ERROR: Unsupported format: ${cur_format} (use deb, rpm, apk, or all)" >&2
            exit 1
            ;;
    esac

    if [[ ! -f "${format_dir}/package.sh" ]]; then
        echo "ERROR: Package script not found: ${format_dir}/package.sh" >&2
        exit 1
    fi

    activate_build_family "${base_image_family}"

    # Clean if requested (once per build family, before iterating architectures).
    # deb and rpm share manylinux_2_28 — skip if already cleaned this run.
    if [[ "${clean}" == "true" ]] && [[ ! " ${cleaned_families:-} " =~ " ${base_image_family} " ]]; then
        echo "==> Cleaning build artifacts..."
        rm -rf "${repo_root}/build" "${repo_root}/.task"
        rm -rf "${repo_root}"/build-* "${repo_root}"/.task-*
        activate_build_family "${base_image_family}"
        cleaned_families="${cleaned_families:-} ${base_image_family}"
    fi

    for target_arch in "${arch_list[@]}"; do
        target_arch=$(echo "${target_arch}" | xargs)

        case "${target_arch}" in
            x86_64)
                docker_suffix="x86_64"
                docker_platform="linux/amd64"
                # Existing image convention uses "x86" (not "x86_64") for x86_64
                image_arch_tag="x86"
                deb_arch="amd64"
                ;;
            aarch64)
                docker_suffix="aarch64"
                docker_platform="linux/arm64"
                image_arch_tag="aarch64"
                deb_arch="arm64"
                ;;
            *)
                echo "ERROR: Unsupported architecture: ${target_arch}" >&2
                exit 1
                ;;
        esac

        # Package arch naming varies by format:
        #   deb: amd64, arm64          (Debian convention)
        #   rpm/apk: x86_64, aarch64   (upstream convention)
        if [[ "${cur_format}" == "deb" ]]; then
            pkg_arch="${deb_arch}"
        else
            pkg_arch="${target_arch}"
        fi

        base_image_tag="clp-core-dependencies-${image_arch_tag}-${base_image_family}:dev"
        builder_image="${builder_image_prefix}-${docker_suffix}:dev"

        echo "========================================"
        echo "Building ${cur_format} for ${target_arch}"
        echo "========================================"

        # Build the base image if not present
        if ! docker image inspect "${base_image_tag}" &>/dev/null; then
            echo "==> Building base image ${base_image_tag}..."
            bash "${repo_root}/components/core/tools/docker-images/clp-env-base-${base_image_family}-${docker_suffix}/build.sh"
        fi

        # Build the builder image (base + packaging tools)
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
        # Run as root inside the container so that:
        #   - Package tools (dpkg-deb, rpmbuild, abuild) produce root-owned files
        #     in the package, which is correct for system binaries
        #   - abuild -F works without fakeroot (avoids musllinux compatibility issues)
        # Disable _FORTIFY_SOURCE for best performance. On Alpine/musllinux this
        # also avoids a GCC LTO incompatibility with fortify-headers.
        # Safe to overwrite CFLAGS/CXXFLAGS since the container has no prior flags.
        docker run --rm \
            --platform "${docker_platform}" \
            -v "${repo_root}:/clp" \
            -w /clp \
            -e "CORES=${cores}" \
            -e "PKG_VERSION=${version}" \
            -e "PKG_ARCH=${pkg_arch}" \
            -e "HOST_UID=$(id -u)" \
            -e "HOST_GID=$(id -g)" \
            -e "CFLAGS=-U_FORTIFY_SOURCE" \
            -e "CXXFLAGS=-U_FORTIFY_SOURCE" \
            "${builder_image}" \
            bash -c '
                set -o errexit
                set -o nounset
                set -o pipefail

                git config --global --add safe.directory "*"

                echo "==> Building dependencies..."
                CLP_CPP_MAX_PARALLELISM_PER_BUILD_TASK="${CORES}" task deps:core

                echo "==> Building core binaries..."
                CLP_CPP_MAX_PARALLELISM_PER_BUILD_TASK="${CORES}" task core

                # BIN_DIR must match the CMake binary output directory (task core
                # builds into /clp/build/core).
                echo "==> Packaging..."
                BIN_DIR=/clp/build/core \
                OUTPUT_DIR=/clp/build \
                    /clp/components/core/tools/packaging/'"${format_dir##*/}"'/package.sh

                # Restore host ownership on mounted volume paths
                chown -R "${HOST_UID}:${HOST_GID}" /clp/build /clp/.task
            '

        # Copy the package to the output directory (only the current format to
        # avoid leaking stale packages of other formats from the build directory)
        echo "==> Copying package to ${output_dir}..."
        find -L "${repo_root}/build" -maxdepth 1 -name "clp-core*.${cur_format}" \
            -exec cp {} "${output_dir}/" \;
        echo ""
    done
done

echo "========================================"
echo "All builds complete!"
echo "========================================"
echo ""
ls -lh "${output_dir}"/clp-core*.{deb,rpm,apk} 2>/dev/null || true
echo ""

for cur_format in "${format_list[@]}"; do
    cur_format=$(echo "${cur_format}" | xargs)
    case "${cur_format}" in
        deb)
            echo "Test deb:"
            echo "  docker run --rm -v '${output_dir}':/pkgs debian:bookworm bash -c \\"
            echo "    'dpkg -i /pkgs/clp-core_*.deb && clp-s --help'"
            ;;
        rpm)
            echo "Test rpm:"
            echo "  docker run --rm -v '${output_dir}':/pkgs almalinux:9 bash -c \\"
            echo "    'rpm -i /pkgs/clp-core-*.rpm && clp-s --help'"
            ;;
        apk)
            echo "Test apk:"
            echo "  docker run --rm -v '${output_dir}':/pkgs alpine:3.20 sh -c \\"
            echo "    'apk add --allow-untrusted /pkgs/clp-core-*.apk && clp-s --help'"
            ;;
    esac
done
