#!/usr/bin/env bash

# Shared output-directory helpers for package builds.

clp_packaging_resolve_output_dir() {
    local output_dir="${1:-}"

    if [[ -z "${output_dir}" ]]; then
        output_dir="${OUTPUT_DIR:-$(pwd)}"
    fi

    if [[ "${output_dir}" == "/" ]]; then
        echo "ERROR: output directory must not be /" >&2
        return 1
    fi

    mkdir -p -- "${output_dir}"
    (cd -- "${output_dir}" && pwd)
}

clp_packaging_remove_stale_outputs() {
    local output_dir="$1"
    local pattern
    shift

    if [[ -z "${output_dir}" || ! -d "${output_dir}" ]]; then
        echo "ERROR: output directory does not exist: '${output_dir}'" >&2
        return 1
    fi

    if [[ "${output_dir}" == "/" ]]; then
        echo "ERROR: refusing to remove stale outputs from /" >&2
        return 1
    fi

    for pattern in "$@"; do
        if [[ -z "${pattern}" ]]; then
            echo "ERROR: stale output pattern must not be empty" >&2
            return 1
        fi

        if [[ "${pattern}" == */* ]]; then
            echo "ERROR: stale output pattern must not contain '/': '${pattern}'" >&2
            return 1
        fi

        find "${output_dir}" -maxdepth 1 \( -type f -o -type l \) -name "${pattern}" -exec rm -f {} +
    done
}
