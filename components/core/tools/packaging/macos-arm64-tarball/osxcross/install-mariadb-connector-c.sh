#!/usr/bin/env bash

# Cross-builds MariaDB Connector/C into the OSXCross dependency prefix.
#
# osxcross-macports currently does not provide mariadb-connector-c for the
# macOS targets CLP needs, but CLP's CMake expects the Connector/C layout:
#   include/mariadb/mysql.h
#   lib/pkgconfig/libmariadb.pc
#   lib/mariadb/libmariadb.dylib

set -o errexit
set -o nounset
set -o pipefail

version="${MARIADB_CONNECTOR_C_VERSION:-3.4.9}"
source_ref="${MARIADB_CONNECTOR_C_REF:-v${version}}"
source_repo="${MARIADB_CONNECTOR_C_REPO:-https://github.com/mariadb-corporation/mariadb-connector-c.git}"
osxcross_target_dir="${OSXCROSS_TARGET_DIR:-/opt/osxcross/target}"
target_bin="${osxcross_target_dir}/bin"
prefix="${OSXCROSS_MACPORTS_ROOT:-${osxcross_target_dir}/macports/pkgs/opt/local}"
toolchain_file="${OSXCROSS_CMAKE_TOOLCHAIN_FILE:-${osxcross_target_dir}/toolchain.cmake}"
deployment_target="${MACOSX_DEPLOYMENT_TARGET:?MACOSX_DEPLOYMENT_TARGET is required}"
work_dir="${MARIADB_CONNECTOR_C_WORK_DIR:-/tmp/mariadb-connector-c}"
macos_arch="${CLP_MACOS_ARCH:-arm64}"

case "${macos_arch}" in
    arm64|aarch64)
        macos_arch="arm64"
        ;;
    x86_64|amd64)
        macos_arch="x86_64"
        ;;
    *)
        echo "ERROR: unsupported CLP_MACOS_ARCH=${macos_arch}" >&2
        echo "       Supported values: arm64, x86_64" >&2
        exit 1
        ;;
esac

if [[ -n "${OSXCROSS_HOST:-}" ]]; then
    target_triple="${OSXCROSS_HOST}"
else
    target_clang="$(find "${target_bin}" -maxdepth 1 -name "${macos_arch}"'-apple-darwin*-clang' \
        | sort -V \
        | tail -1)"
    if [[ -z "${target_clang}" ]]; then
        echo "ERROR: could not detect OSXCross ${macos_arch} target triple in ${target_bin}" >&2
        exit 1
    fi
    target_triple="$(basename "${target_clang%-clang}")"
fi

osxcross_conf="${target_bin}/${target_triple}-osxcross-conf"
if [[ -x "${osxcross_conf}" ]]; then
    eval "$("${osxcross_conf}")"
fi

export OSXCROSS_HOST="${target_triple}"
export OSXCROSS_TARGET_DIR="${OSXCROSS_TARGET_DIR:-${osxcross_target_dir}}"
export OSXCROSS_TARGET="${OSXCROSS_TARGET:-${target_triple#*-apple-}}"
if [[ -z "${OSXCROSS_SDK:-}" ]]; then
    OSXCROSS_SDK="$(find "${OSXCROSS_TARGET_DIR}/SDK" -maxdepth 1 -type d -name 'MacOSX*.sdk' \
        | sort -V \
        | tail -1)"
fi
if [[ -z "${OSXCROSS_SDK}" || ! -d "${OSXCROSS_SDK}" ]]; then
    echo "ERROR: could not find a macOS SDK inside ${OSXCROSS_TARGET_DIR}/SDK." >&2
    echo "       Select an OSXCross base image that already includes the SDK/toolchain." >&2
    exit 1
fi
export OSXCROSS_SDK

if [[ ! -f "${toolchain_file}" ]]; then
    echo "ERROR: OSXCross CMake toolchain file not found: ${toolchain_file}" >&2
    exit 1
fi

if [[ ! -d "${prefix}" ]]; then
    echo "ERROR: OSXCross dependency prefix not found: ${prefix}" >&2
    exit 1
fi

pkg_config_dirs=("${prefix}/lib/pkgconfig" "${prefix}/share/pkgconfig")
cmake_prefix_paths=("${prefix}")
cmake_find_root_paths=("${prefix}")
openssl_cmake_args=()

for openssl_candidate in \
    "${prefix}/libexec/openssl3" \
    "${prefix}/libexec/openssl11" \
    "${prefix}/opt/openssl3" \
    "${prefix}/opt/openssl"; do
    if [[ -d "${openssl_candidate}" ]]; then
        cmake_prefix_paths+=("${openssl_candidate}")
        cmake_find_root_paths+=("${openssl_candidate}")
        if [[ -d "${openssl_candidate}/lib/pkgconfig" ]]; then
            pkg_config_dirs+=("${openssl_candidate}/lib/pkgconfig")
        fi
        openssl_cmake_args+=(
            "-DOPENSSL_ROOT_DIR=${openssl_candidate}"
            "-DOPENSSL_INCLUDE_DIR=${openssl_candidate}/include"
            "-DOPENSSL_SSL_LIBRARY=${openssl_candidate}/lib/libssl.dylib"
            "-DOPENSSL_CRYPTO_LIBRARY=${openssl_candidate}/lib/libcrypto.dylib"
        )
        break
    fi
done

join_by() {
    local delimiter="$1"
    shift
    local joined=""
    local item
    for item in "$@"; do
        if [[ -z "${joined}" ]]; then
            joined="${item}"
        else
            joined="${joined}${delimiter}${item}"
        fi
    done
    printf '%s\n' "${joined}"
}

rm -rf "${work_dir}"
mkdir -p "${work_dir}"

echo "==> Fetching MariaDB Connector/C ${source_ref}..."
git clone --depth 1 --branch "${source_ref}" "${source_repo}" "${work_dir}/src"

export PKG_CONFIG_LIBDIR="$(join_by ':' "${pkg_config_dirs[@]}")"
export PKG_CONFIG_SYSROOT_DIR="${OSXCROSS_SDKROOT:-}"

echo "==> Configuring MariaDB Connector/C..."
cmake \
    -S "${work_dir}/src" \
    -B "${work_dir}/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="${toolchain_file}" \
    -DCMAKE_SYSTEM_NAME=Darwin \
    -DCMAKE_OSX_ARCHITECTURES="${macos_arch}" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${deployment_target}" \
    -DCMAKE_INSTALL_PREFIX="${prefix}" \
    -DCMAKE_PREFIX_PATH="$(join_by ';' "${cmake_prefix_paths[@]}")" \
    -DCMAKE_FIND_ROOT_PATH="$(join_by ';' "${cmake_find_root_paths[@]}")" \
    -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH \
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH \
    -DINSTALL_LAYOUT=DEFAULT \
    -DWITH_UNIT_TESTS=OFF \
    -DWITH_DOCS=OFF \
    -DWITH_CURL=OFF \
    -DWITH_EXTERNAL_ZLIB=ON \
    -DWITH_SSL=OPENSSL \
    "${openssl_cmake_args[@]}"

echo "==> Building MariaDB Connector/C..."
cmake --build "${work_dir}/build" --parallel "$(nproc 2>/dev/null || echo 4)"

echo "==> Installing MariaDB Connector/C into ${prefix}..."
cmake --install "${work_dir}/build"

pkg_config_file="${prefix}/lib/pkgconfig/libmariadb.pc"
header_file="${prefix}/include/mariadb/mysql.h"
library_file="${prefix}/lib/mariadb/libmariadb.dylib"
for required_file in "${pkg_config_file}" "${header_file}" "${library_file}"; do
    if [[ ! -e "${required_file}" ]]; then
        echo "ERROR: MariaDB Connector/C install did not produce ${required_file}" >&2
        exit 1
    fi
done
sed -i 's#^prefix=.*#prefix=/opt/local#' "${pkg_config_file}"

rm -rf "${work_dir}"
echo "==> MariaDB Connector/C ${version} installed."
