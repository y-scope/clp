#!/usr/bin/env bash

# Shared validation helpers for dependency-bundling scripts.

clp_packaging_validate_bundle_inputs() {
    if [[ -z "${DESTDIR:-}" ]]; then
        echo "ERROR: DESTDIR is required" >&2
        return 1
    fi

    if [[ "${DESTDIR}" != /* ]]; then
        echo "ERROR: DESTDIR must be an absolute path, got: '${DESTDIR}'" >&2
        return 1
    fi

    if [[ "${DESTDIR}" == "/" ]]; then
        echo "ERROR: DESTDIR must not be /" >&2
        return 1
    fi

    if [[ -z "${BIN_DIR:-}" ]]; then
        echo "ERROR: BIN_DIR is required" >&2
        return 1
    fi

    if [[ ! -d "${BIN_DIR}" ]]; then
        echo "ERROR: Binary directory not found: '${BIN_DIR}'" >&2
        return 1
    fi
}

clp_packaging_resolve_bundle_prefix() {
    local default_prefix="$1"
    local prefix="${PREFIX-${default_prefix}}"

    if [[ -n "${prefix}" && "${prefix}" != /* ]]; then
        echo "ERROR: PREFIX must start with '/' or be empty, got: '${prefix}'" >&2
        return 1
    fi

    printf '%s\n' "${prefix}"
}
