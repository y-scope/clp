#!/usr/bin/env bash
set -euo pipefail

files=()
while [[ "$#" -gt 0 ]]; do
    if [[ "$1" == "--" ]]; then
        shift
        break
    fi
    files+=("$1")
    shift
done

if [[ "${#files[@]}" -eq 0 || "$#" -eq 0 ]]; then
    echo "Usage: $0 <file>... -- <command> [args...]" >&2
    exit 1
fi

for _ in $(seq 1 "${CLP_SINGLE_CONTAINER_WAIT_ATTEMPTS:-120}"); do
    all_files_exist=true
    for file in "${files[@]}"; do
        if [[ ! -e "$file" ]]; then
            all_files_exist=false
            break
        fi
    done

    if [[ "$all_files_exist" == "true" ]]; then
        exec "$@"
    fi

    sleep 1
done

echo "Timed out waiting for files: ${files[*]}" >&2
exit 1
