#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

if ! command -v pipx >/dev/null 2>&1; then
    echo "Error: pipx not found."
    exit 1
fi

if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq not found."
    exit 1
fi

if [ "$#" -ne 2 ] || [ -z "${1:-}" ] || [ -z "${2:-}" ]; then
  echo "Usage: $0 <package_name> <binary_name>" >&2
  exit 2
fi

pkg="$1"
app="$2"

pipx list --json | jq --raw-output --arg pkg "$pkg" --arg app "$app" '
	.venvs[$pkg].metadata.main_package.app_paths[]?.__Path__
	| select((split("/") | last) == $app)
' | head -n1
