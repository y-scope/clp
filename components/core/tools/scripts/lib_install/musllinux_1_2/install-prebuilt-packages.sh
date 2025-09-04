#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

apk update && apk add --no-cache \
    bzip2-dev \
    bzip2-static \
    curl-dev \
    jq \
    mariadb-connector-c-dev \
    openjdk11-jdk \
    openssl-dev \
    zlib-dev \
    zlib-static

# Install `cmake`, `go-task` and `uv`
"${script_dir}/../pipx_install/install-all.sh"
