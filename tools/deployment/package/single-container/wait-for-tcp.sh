#!/usr/bin/env bash
set -euo pipefail

host="$1"
port="$2"
shift 2

for _ in $(seq 1 120); do
    if timeout 1 bash -c "</dev/tcp/${host}/${port}" >/dev/null 2>&1; then
        exec "$@"
    fi
    sleep 1
done

echo "Timed out waiting for ${host}:${port}" >&2
exit 1
