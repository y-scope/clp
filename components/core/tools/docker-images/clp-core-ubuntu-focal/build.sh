#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

cUsage="Usage: ${BASH_SOURCE[0]} <clp-core-build-dir>"
if [ "$#" -lt 1 ]; then
    echo "${cUsage}"
    exit
fi
build_dir="$1"

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
docker build -t clp-core-x86-ubuntu-focal:dev "${build_dir}" --file "${script_dir}/Dockerfile"
