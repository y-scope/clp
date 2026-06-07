#!/usr/bin/env bash

# Shared validation helpers for package-format scripts.

clp_packaging_validate_package_inputs() {
    local package_name="$1"
    local expected_arch="${2:-}"

    if [[ -z "${PKG_VERSION:-}" ]]; then
        echo "ERROR: PKG_VERSION is required" >&2
        return 1
    fi

    if [[ -z "${PKG_ARCH:-}" ]]; then
        echo "ERROR: PKG_ARCH is required" >&2
        return 1
    fi

    if [[ -n "${expected_arch}" && "${PKG_ARCH}" != "${expected_arch}" ]]; then
        echo "ERROR: ${package_name} only supports PKG_ARCH=${expected_arch}, got: '${PKG_ARCH}'" >&2
        return 1
    fi

    if [[ -z "${BIN_DIR:-}" ]]; then
        echo "ERROR: BIN_DIR is required" >&2
        return 1
    fi
}

clp_packaging_output_dir() {
    printf '%s\n' "${OUTPUT_DIR:-$(pwd)}"
}
