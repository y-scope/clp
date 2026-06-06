#!/usr/bin/env bash

# Builds the macOS tarball locally using a Dockerized OSXCross toolchain.
#
# This local-dev path derives a CLP-specific toolchain image from a prebuilt
# OSXCross image and then runs that image against the mounted repository. It
# intentionally does not accept or package a macOS SDK; the base image supplies
# the OSXCross SDK/toolchain.

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
repo_root="$(cd "${script_dir}/../../../../.." && pwd)"
component_root="${repo_root}/components/core"

# shellcheck source=defaults.sh
. "${script_dir}/defaults.sh"

base_image="${OSXCROSS_BASE_IMAGE:-ghcr.io/crazy-max/osxcross:14.5-r0-ubuntu}"
image_tag="${CLP_MACOS_OSXCROSS_IMAGE_TAG:-}"
builder_platform="${OSXCROSS_BUILDER_PLATFORM:-}"
macosx_deployment_target="${MACOSX_DEPLOYMENT_TARGET:-${CLP_MACOS_DEFAULT_DEPLOYMENT_TARGET}}"
macos_arch="$(clp_macos_normalize_arch "${CLP_MACOS_ARCH:-${CLP_MACOS_DEFAULT_ARCH}}")"
mariadb_connector_c_version="${MARIADB_CONNECTOR_C_VERSION:-3.4.9}"
host_output_dir=""
build_image=true
docker_network_args=()
build_args=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --base-image)
            [[ -n "${2:-}" ]] || { echo "ERROR: --base-image requires a value" >&2; exit 1; }
            base_image="$2"; shift 2 ;;
        --image-tag|--toolchain-image)
            [[ -n "${2:-}" ]] || { echo "ERROR: $1 requires a value" >&2; exit 1; }
            image_tag="$2"; shift 2 ;;
        --builder-platform)
            [[ -n "${2:-}" ]] || { echo "ERROR: --builder-platform requires a value" >&2; exit 1; }
            builder_platform="$2"; shift 2 ;;
        --arch)
            [[ -n "${2:-}" ]] || { echo "ERROR: --arch requires a value" >&2; exit 1; }
            macos_arch="$(clp_macos_normalize_arch "$2")"; shift 2 ;;
        --deployment-target)
            [[ -n "${2:-}" ]] || { echo "ERROR: --deployment-target requires a value" >&2; exit 1; }
            macosx_deployment_target="$2"
            build_args+=("$1" "$2")
            shift 2 ;;
        --sdk|--sdk-url|--sdk-sha256|--sdk-version|--manylinux-base-image)
            echo "ERROR: $1 is no longer supported by this local build flow." >&2
            echo "       The OSXCross base image supplies the macOS SDK/toolchain." >&2
            echo "       Use --base-image to select a different prebuilt OSXCross image." >&2
            exit 1 ;;
        --no-build-image)
            build_image=false; shift ;;
        --cores|--version|--clean)
            build_args+=("$1")
            if [[ "$1" != "--clean" ]]; then
                [[ -n "${2:-}" ]] || { echo "ERROR: $1 requires a value" >&2; exit 1; }
                build_args+=("$2")
                shift 2
            else
                shift
            fi ;;
        --output)
            [[ -n "${2:-}" ]] || { echo "ERROR: --output requires a value" >&2; exit 1; }
            host_output_dir="$2"; shift 2 ;;
        --help)
            sed -nE '/^# Builds/,/^$/p' "$0"
            echo "Options: --base-image IMAGE --image-tag TAG --toolchain-image TAG --builder-platform PLATFORM --arch ARCH --deployment-target VER --no-build-image --cores N --version VER --output DIR --clean"
            exit 0 ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1 ;;
    esac
done

for legacy_env_var in \
    OSXCROSS_SDK_TARBALL \
    OSXCROSS_SDK_TARBALL_URL \
    OSXCROSS_SDK_TARBALL_SHA256 \
    OSXCROSS_SDK_VERSION \
    OSXCROSS_MANYLINUX_BASE_IMAGE \
    OSXCROSS_REVISION; do
    if [[ -n "${!legacy_env_var:-}" ]]; then
        echo "ERROR: ${legacy_env_var} is no longer supported by this local build flow." >&2
        echo "       The OSXCross base image supplies the macOS SDK/toolchain." >&2
        echo "       Use OSXCROSS_BASE_IMAGE or --base-image to select a different base image." >&2
        exit 1
    fi
done

if ! command -v docker >/dev/null 2>&1; then
    echo "ERROR: docker is required" >&2
    exit 1
fi

docker_server_os="$(docker version --format '{{.Server.Os}}' 2>/dev/null || true)"
docker_server_arch="$(docker version --format '{{.Server.Arch}}' 2>/dev/null || true)"
if [[ "${docker_server_os}" != "linux" ]]; then
    echo "ERROR: build-cross.sh requires a Linux Docker server (got: ${docker_server_os:-unknown})." >&2
    echo "       On macOS, run this with Docker Desktop or use build.sh --native for native builds." >&2
    exit 1
fi

if [[ -z "${builder_platform}" ]]; then
    case "${docker_server_arch}" in
        amd64|x86_64)
            builder_platform="linux/amd64"
            ;;
        arm64|aarch64)
            builder_platform="linux/arm64"
            ;;
        *)
            echo "ERROR: unsupported Docker server architecture: ${docker_server_arch:-unknown}" >&2
            echo "       Set --builder-platform explicitly." >&2
            exit 1
            ;;
    esac
fi

if [[ -n "${DOCKER_NETWORK:-}" ]]; then
    docker_network_args=(--network "${DOCKER_NETWORK}")
fi

if [[ -z "${image_tag}" ]]; then
    image_tag="clp-macos-${macos_arch}-osxcross:dev"
fi
osxcross_macports_packages="${OSXCROSS_MACPORTS_PACKAGES:-$(clp_macos_osxcross_macports_packages "${macos_arch}")}"

if [[ "${build_image}" == "true" ]]; then
    # Reuse the same corporate proxy / CA setup as the Linux packaging images.
    # The Docker build command is assembled here so all flags are emitted before
    # the build context.
    source "${repo_root}/components/core/tools/scripts/corporate-proxy-host.sh"
    prepare_ca_cert_for_build "${component_root}"
    trap 'cleanup_ca_cert "${component_root}"' EXIT

    context_dir="$(mktemp -d -t clp-osxcross-context.XXXXXX)"
    trap 'rm -rf "${context_dir}"; cleanup_ca_cert "${component_root}"' EXIT

    mkdir -p "${context_dir}/components/core/tools/packaging/macos-arm64-tarball/osxcross"
    mkdir -p "${context_dir}/components/core/tools/scripts/lib_install/pipx-packages"
    mkdir -p "${context_dir}/tools/scripts"
    cp "${script_dir}/osxcross/Dockerfile" "${context_dir}/Dockerfile"
    cp "${script_dir}/osxcross/cmake-wrapper.sh" \
        "${context_dir}/components/core/tools/packaging/macos-arm64-tarball/osxcross/cmake-wrapper.sh"
    cp "${script_dir}/osxcross/install-mariadb-connector-c.sh" \
        "${context_dir}/components/core/tools/packaging/macos-arm64-tarball/osxcross/install-mariadb-connector-c.sh"
    cp -R "${repo_root}/components/core/tools/scripts/lib_install/pipx-packages/." \
        "${context_dir}/components/core/tools/scripts/lib_install/pipx-packages/"
    cp "${component_root}/tools/scripts/corporate-proxy-container.sh" \
        "${context_dir}/tools/scripts/corporate-proxy-container.sh"
    cp "${component_root}/tools/scripts/ca-certificates.crt" \
        "${context_dir}/tools/scripts/ca-certificates.crt"

    build_cmd=(docker build)
    if [[ -n "${builder_platform}" ]]; then
        build_cmd+=(--platform "${builder_platform}")
    fi
    build_cmd+=(
        --build-arg "OSXCROSS_BASE_IMAGE=${base_image}"
        --build-arg "CLP_MACOS_ARCH=${macos_arch}"
        --build-arg "MACOSX_DEPLOYMENT_TARGET=${macosx_deployment_target}"
        --build-arg "OSXCROSS_MACPORTS_PACKAGES=${osxcross_macports_packages}"
        --build-arg "MARIADB_CONNECTOR_C_VERSION=${mariadb_connector_c_version}"
        --tag "${image_tag}"
    )
    add_proxy_build_args build_cmd
    if [[ "${DOCKER_PULL:-true}" != "false" ]]; then
        build_cmd+=(--pull)
    fi
    if command -v git >/dev/null \
        && git -C "${script_dir}" rev-parse --is-inside-work-tree >/dev/null 2>&1
    then
        revision="$(git -C "${script_dir}" rev-parse HEAD 2>/dev/null)"
        build_cmd+=(--label "org.opencontainers.image.revision=${revision}")

        if remote_url="$(git -C "${script_dir}" remote get-url origin 2>/dev/null)"; then
            build_cmd+=(--label "org.opencontainers.image.source=${remote_url}")
        fi
    fi
    build_cmd+=("${context_dir}")
    echo "Running: ${build_cmd[*]}"
    "${build_cmd[@]}"
fi

if [[ -n "${host_output_dir}" ]]; then
    host_output_dir="$(mkdir -p "${host_output_dir}" && cd "${host_output_dir}" && pwd)"
fi

mkdir -p "${repo_root}/packages"

run_cmd=(
    docker run --rm
)
if [[ ${#docker_network_args[@]} -gt 0 ]]; then
    run_cmd+=("${docker_network_args[@]}")
fi
if [[ -n "${builder_platform}" ]]; then
    run_cmd+=(--platform "${builder_platform}")
fi
run_cmd+=(
    --user "$(id -u):$(id -g)"
    -e "CLP_MACOS_ARCH=${macos_arch}"
    -e "MACOSX_DEPLOYMENT_TARGET=${macosx_deployment_target}"
    -e "HOME=/tmp/clp-osxcross-home"
    -e "HOST_UID=$(id -u)"
    -e "HOST_GID=$(id -g)"
    -v "${repo_root}:/clp"
)
for osxcross_env_var in OSXCROSS_ALLOW_UNSIGNED OSXCROSS_CODESIGN; do
    if [[ -n "${!osxcross_env_var:-}" ]]; then
        run_cmd+=(-e "${osxcross_env_var}=${!osxcross_env_var}")
    fi
done
if [[ -n "${host_output_dir}" ]]; then
    run_cmd+=(-v "${host_output_dir}:/clp-output")
    build_args+=(--output /clp-output)
fi
run_cmd+=(
    -w /clp
    "${image_tag}"
    components/core/tools/packaging/macos-arm64-tarball/build-in-osxcross-container.sh
)
if [[ ${#build_args[@]} -gt 0 ]]; then
    run_cmd+=("${build_args[@]}")
fi

"${run_cmd[@]}"
