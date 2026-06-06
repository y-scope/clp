#!/usr/bin/env bash

# Shared defaults for macOS tarball builds.

# Preserve the original local-build default. Native builds override this with
# the host architecture when --arch is not supplied.
CLP_MACOS_DEFAULT_ARCH="arm64"

# Controls the minimum macOS version the produced binaries should run on.
# The SDK version controls available headers/stubs; it does not define the
# runtime floor.
CLP_MACOS_DEFAULT_DEPLOYMENT_TARGET="14.0"

clp_macos_normalize_arch() {
    local arch="$1"

    case "${arch}" in
        arm64|aarch64)
            printf 'arm64\n'
            ;;
        x86_64|amd64)
            printf 'x86_64\n'
            ;;
        *)
            echo "ERROR: unsupported macOS architecture: ${arch}" >&2
            echo "       Supported values: arm64, x86_64" >&2
            return 1
            ;;
    esac
}

clp_macos_osxcross_target_arch() {
    clp_macos_normalize_arch "$1"
}

clp_macos_osxcross_macports_packages() {
    local arch

    arch="$(clp_macos_normalize_arch "$1")"
    case "${arch}" in
        arm64)
            printf '%s\n' '-arm64 libarchive openssl'
            ;;
        x86_64)
            printf '%s\n' 'libarchive openssl'
            ;;
    esac
}

clp_macos_boost_architecture_arg() {
    local arch

    arch="$(clp_macos_normalize_arch "$1")"
    case "${arch}" in
        arm64)
            printf '%s\n' 'architecture=arm'
            ;;
        x86_64)
            printf '%s\n' 'architecture=x86'
            ;;
    esac
}

clp_macos_boost_abi_arg() {
    local arch

    arch="$(clp_macos_normalize_arch "$1")"
    case "${arch}" in
        arm64)
            printf '%s\n' 'abi=aapcs'
            ;;
        x86_64)
            printf '%s\n' 'abi=sysv'
            ;;
    esac
}
