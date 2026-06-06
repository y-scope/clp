#!/usr/bin/env bash

# Runs inside the Dockerized OSXCross image. Produces the same tarball format
# as the native macOS build, but targets macOS from Linux.

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
repo_root="$(cd "${script_dir}/../../../../.." && pwd)"

# shellcheck source=defaults.sh
. "${script_dir}/defaults.sh"

export MACOSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-${CLP_MACOS_DEFAULT_DEPLOYMENT_TARGET}}"
args=("$@")
macos_arch_input="${CLP_MACOS_ARCH:-${CLP_MACOS_DEFAULT_ARCH}}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --help)
            exec "${script_dir}/build-package.sh" --help
            ;;
        --arch)
            [[ -n "${2:-}" ]] || { echo "ERROR: --arch requires a value" >&2; exit 1; }
            macos_arch_input="$2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done
set -- "${args[@]}"

macos_arch="$(clp_macos_normalize_arch "${macos_arch_input}")"
target_arch="$(clp_macos_osxcross_target_arch "${macos_arch}")"

if [[ "$(uname -s)" != "Linux" ]]; then
    echo "ERROR: build-in-osxcross-container.sh must run on Linux" >&2
    exit 1
fi

if ! command -v git >/dev/null 2>&1; then
    echo "ERROR: required tool not found: git" >&2
    exit 1
fi

mkdir -p "${HOME}"

osxcross_target_dir="${OSXCROSS_TARGET_DIR:-/opt/osxcross/target}"
target_bin="${osxcross_target_dir}/bin"

if [[ -n "${OSXCROSS_TARGET_TRIPLE:-}" ]]; then
    target_triple="${OSXCROSS_TARGET_TRIPLE}"
else
    target_clang="$(find "${target_bin}" -maxdepth 1 -name "${target_arch}"'-apple-darwin*-clang' \
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

cc="${OSXCROSS_CC:-${target_bin}/${target_triple}-clang}"
cxx="${OSXCROSS_CXX:-${target_bin}/${target_triple}-clang++}"
ar="${OSXCROSS_AR:-${target_bin}/${target_triple}-ar}"
ranlib="${OSXCROSS_RANLIB:-${target_bin}/${target_triple}-ranlib}"
libtool="${OSXCROSS_LIBTOOL:-${target_bin}/${target_triple}-libtool}"
install_name_tool="${OSXCROSS_INSTALL_NAME_TOOL:-${target_bin}/${target_triple}-install_name_tool}"
otool="${OSXCROSS_OTOOL:-${target_bin}/${target_triple}-otool}"
strip_tool="${OSXCROSS_STRIP:-${target_bin}/${target_triple}-strip}"
if [[ -n "${OSXCROSS_CODESIGN:-}" ]]; then
    codesign_tool="${OSXCROSS_CODESIGN}"
elif command -v "${target_bin}/${target_triple}-codesign" >/dev/null 2>&1 \
        || [[ -x "${target_bin}/${target_triple}-codesign" ]]; then
    codesign_tool="${target_bin}/${target_triple}-codesign"
elif command -v ldid >/dev/null 2>&1; then
    codesign_tool="$(command -v ldid)"
else
    codesign_tool="skip"
fi
if [[ "${codesign_tool}" == "skip" && "${OSXCROSS_ALLOW_UNSIGNED:-false}" != "true" ]]; then
    echo "ERROR: no Mach-O signing tool found." >&2
    echo "       Install ldid, provide OSXCross codesign, or set OSXCROSS_CODESIGN." >&2
    echo "       To produce an unsigned test artifact anyway, set OSXCROSS_ALLOW_UNSIGNED=true." >&2
    exit 1
fi
toolchain_file="${OSXCROSS_CMAKE_TOOLCHAIN_FILE:-${osxcross_target_dir}/toolchain.cmake}"
if [[ -n "${OSXCROSS_MACPORTS_ROOT:-}" ]]; then
    macports_root="${OSXCROSS_MACPORTS_ROOT}"
else
    macports_root=""
    for candidate in \
        "${osxcross_target_dir}/macports/pkgs/opt/local" \
        "${osxcross_target_dir}/macports/opt/local" \
        "${osxcross_target_dir}/macdeps" \
        "${osxcross_target_dir}"; do
        if [[ -d "${candidate}/lib/pkgconfig" || -d "${candidate}/share/pkgconfig" ]]; then
            macports_root="${candidate}"
            break
        fi
    done
fi

for tool in "${cc}" "${cxx}" "${ar}" "${ranlib}" "${libtool}" "${install_name_tool}" "${otool}" "${strip_tool}" task git python3; do
    if ! command -v "${tool}" >/dev/null 2>&1 && [[ ! -x "${tool}" ]]; then
        echo "ERROR: required tool not found: ${tool}" >&2
        exit 1
    fi
done

if [[ ! -f "${toolchain_file}" ]]; then
    echo "ERROR: OSXCross CMake toolchain file not found: ${toolchain_file}" >&2
    exit 1
fi

if [[ -z "${macports_root}" ]]; then
    echo "ERROR: could not detect OSXCross dependency prefix." >&2
    echo "       Set OSXCROSS_MACPORTS_ROOT to the prefix containing lib/pkgconfig." >&2
    exit 1
fi
export OSXCROSS_MACPORTS_ROOT="${macports_root}"

openssl_root=""
for candidate in \
    "${macports_root}/libexec/openssl3" \
    "${macports_root}/libexec/openssl11" \
    "${macports_root}/opt/openssl3" \
    "${macports_root}/opt/openssl"; do
    if [[ -d "${candidate}" ]]; then
        openssl_root="${candidate}"
        break
    fi
done

pkg_config_dirs=("${macports_root}/lib/pkgconfig" "${macports_root}/share/pkgconfig")
if [[ -n "${openssl_root}" && -d "${openssl_root}/lib/pkgconfig" ]]; then
    pkg_config_dirs+=("${openssl_root}/lib/pkgconfig")
fi

osxcross_build_family="macos-${macos_arch}-cross"
osxcross_build_family_dir="${repo_root}/build-${osxcross_build_family}"
mkdir -p "${osxcross_build_family_dir}"
osxcross_toolchain_wrapper="${osxcross_build_family_dir}/osxcross-toolchain.cmake"

mariadb_pkg_config_file="${macports_root}/lib/pkgconfig/libmariadb.pc"
if [[ -f "${mariadb_pkg_config_file}" ]]; then
    pkg_config_overlay_dir="${osxcross_build_family_dir}/pkgconfig"
    mkdir -p "${pkg_config_overlay_dir}"
    cp "${mariadb_pkg_config_file}" "${pkg_config_overlay_dir}/libmariadb.pc"
    sed -i 's#^prefix=.*#prefix=/opt/local#' "${pkg_config_overlay_dir}/libmariadb.pc"
    pkg_config_dirs=("${pkg_config_overlay_dir}" "${pkg_config_dirs[@]}")
fi

pkg_config_libdir=""
for pkg_config_dir in "${pkg_config_dirs[@]}"; do
    if [[ -z "${pkg_config_libdir}" ]]; then
        pkg_config_libdir="${pkg_config_dir}"
    else
        pkg_config_libdir="${pkg_config_libdir}:${pkg_config_dir}"
    fi
done

export CC="${cc}"
export CXX="${cxx}"
export AR="${ar}"
export RANLIB="${ranlib}"
export PKG_CONFIG_LIBDIR="${pkg_config_libdir}"
export PKG_CONFIG_SYSROOT_DIR="${osxcross_target_dir}/macports/pkgs"

export CLP_MACOS_ARCH="${macos_arch}"
export CLP_MACOS_BUILD_FAMILY="${osxcross_build_family}"
export CLP_MACOS_BUILD_TITLE="CLP macOS ${macos_arch} OSXCross Tarball Build"
export CLP_MACOS_BUILD_DETAILS
CLP_MACOS_BUILD_DETAILS="$(printf '    Target:   %s\n    Deps:     %s' \
    "${target_triple}" "${macports_root}")"
export CLP_MACOS_CLEAN_EXTRA_PATHS="${repo_root}/build/deps"
export CLP_MACOS_POST_DEPS_HOOK="${script_dir}/osxcross/sanitize-deps-metadata.sh"
export CLP_MACOS_POST_DEPS_LABEL="Sanitising cross-built dependency metadata"

export MACHO_OTOOL="${otool}"
export MACHO_INSTALL_NAME_TOOL="${install_name_tool}"
export MACHO_CODESIGN="${codesign_tool}"
export MACHO_ALLOW_UNSIGNED="${OSXCROSS_ALLOW_UNSIGNED:-false}"
export MACHO_STRIP="${strip_tool}"

cat > "${osxcross_toolchain_wrapper}" <<EOF
include("${toolchain_file}")

set(_clp_osxcross_deps_root "${repo_root}/build/deps/cpp")
set(_clp_osxcross_macports_root "${macports_root}")
set(_clp_osxcross_openssl_root "${openssl_root}")
set(_clp_osxcross_sdk "${OSXCROSS_SDK}")
set(_clp_osxcross_pkg_config_libdir "${pkg_config_libdir}")
set(_clp_osxcross_pkg_config_sysroot "${osxcross_target_dir}/macports/pkgs")

set(ENV{PKG_CONFIG_LIBDIR} "\${_clp_osxcross_pkg_config_libdir}")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "\${_clp_osxcross_pkg_config_sysroot}")

file(GLOB _clp_osxcross_dep_prefixes LIST_DIRECTORIES true
  "\${_clp_osxcross_deps_root}/*-install")
if(_clp_osxcross_dep_prefixes)
  list(PREPEND CMAKE_PREFIX_PATH \${_clp_osxcross_dep_prefixes})
  list(PREPEND CMAKE_FIND_ROOT_PATH \${_clp_osxcross_dep_prefixes})
endif()
if(IS_DIRECTORY "\${_clp_osxcross_macports_root}")
  list(PREPEND CMAKE_PREFIX_PATH "\${_clp_osxcross_macports_root}")
  list(PREPEND CMAKE_FIND_ROOT_PATH "\${_clp_osxcross_macports_root}")
endif()
if(IS_DIRECTORY "\${_clp_osxcross_sdk}/usr")
  list(PREPEND CMAKE_PREFIX_PATH "\${_clp_osxcross_sdk}/usr")
  list(PREPEND CMAKE_FIND_ROOT_PATH "\${_clp_osxcross_sdk}")
endif()
if(IS_DIRECTORY "\${_clp_osxcross_openssl_root}")
  list(PREPEND CMAKE_PREFIX_PATH "\${_clp_osxcross_openssl_root}")
  list(PREPEND CMAKE_FIND_ROOT_PATH "\${_clp_osxcross_openssl_root}")
  set(OPENSSL_ROOT_DIR "\${_clp_osxcross_openssl_root}" CACHE PATH "OpenSSL root." FORCE)
  set(OPENSSL_INCLUDE_DIR "\${_clp_osxcross_openssl_root}/include" CACHE PATH "OpenSSL include dir." FORCE)
  set(OPENSSL_SSL_LIBRARY "\${_clp_osxcross_openssl_root}/lib/libssl.dylib" CACHE FILEPATH "OpenSSL ssl library." FORCE)
  set(OPENSSL_CRYPTO_LIBRARY "\${_clp_osxcross_openssl_root}/lib/libcrypto.dylib" CACHE FILEPATH "OpenSSL crypto library." FORCE)
endif()
if(EXISTS "\${_clp_osxcross_sdk}/usr/include/curl/curl.h"
    AND EXISTS "\${_clp_osxcross_sdk}/usr/lib/libcurl.tbd")
  # CMake's FindCURL does not reliably discover SDK .tbd stubs through the
  # OSXCross sysroot, so point it at the SDK-provided libcurl explicitly.
  set(CURL_INCLUDE_DIR "\${_clp_osxcross_sdk}/usr/include" CACHE PATH "SDK curl include dir." FORCE)
  set(CURL_LIBRARY "\${_clp_osxcross_sdk}/usr/lib/libcurl.tbd" CACHE FILEPATH "SDK libcurl stub." FORCE)
endif()
if(CMAKE_PREFIX_PATH)
  list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)
endif()
if(CMAKE_FIND_ROOT_PATH)
  list(REMOVE_DUPLICATES CMAKE_FIND_ROOT_PATH)
endif()

# Cross-configured dependencies must not discover Linux host libraries or
# headers. Otherwise generated CMake/pkg-config metadata can record paths like
# /usr/lib64/librt.so, which cannot be linked into a Mach-O binary.
list(APPEND CMAKE_IGNORE_PATH
  "/lib"
  "/lib64"
  "/lib/aarch64-linux-gnu"
  "/lib/x86_64-linux-gnu"
  "/usr/include"
  "/usr/lib"
  "/usr/lib64"
  "/usr/lib/aarch64-linux-gnu"
  "/usr/lib/x86_64-linux-gnu"
  "/usr/local/include"
  "/usr/local/lib"
)
list(REMOVE_DUPLICATES CMAKE_IGNORE_PATH)

set(LIBRT "" CACHE FILEPATH "Darwin does not provide librt." FORCE)
set(RT_LIBRARY "" CACHE FILEPATH "Darwin does not provide librt." FORCE)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
EOF
export CLP_OSXCROSS_CMAKE_ARGS="-DCMAKE_TOOLCHAIN_FILE=${osxcross_toolchain_wrapper} -DCMAKE_SYSTEM_NAME=Darwin -DCMAKE_OSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET}"
# Boost's clang-darwin toolset can inject a partial --target, which makes
# OSXCross's clang wrapper fall back to the host linker. The darwin toolset lets
# the wrapper infer the full target triple from argv[0].
export CLP_OSXCROSS_BOOST_TOOLSET_ARG="toolset=darwin"
export CLP_OSXCROSS_BOOST_TARGET_OS_ARG="target-os=darwin"
export CLP_OSXCROSS_BOOST_ARCHITECTURE_ARG="$(clp_macos_boost_architecture_arg "${macos_arch}")"
export CLP_OSXCROSS_BOOST_ABI_ARG="$(clp_macos_boost_abi_arg "${macos_arch}")"
export CLP_OSXCROSS_BOOST_ADDRESS_MODEL_ARG="address-model=64"
export CLP_OSXCROSS_BOOST_BINARY_FORMAT_ARG="binary-format=mach-o"
export CLP_OSXCROSS_ANTLR_BUILD_CPP_TESTS_ARG="-DANTLR_BUILD_CPP_TESTS=OFF"

cat > "${HOME}/user-config.jam" <<EOF
using darwin : : ${cxx} : <archiver>${libtool} ;
EOF

exec "${script_dir}/build-package.sh" "$@"
