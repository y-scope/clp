#!/usr/bin/env bash

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
common_env_path="$script_dir/.common-env.sh"

# shellcheck source=.common-env.sh
source "$common_env_path"

docker compose -f "$CLP_HOME/docker-compose.runtime.yaml" run --rm clp-runtime \
    python3 \
    -m clp_package_utils.scripts.compress_from_s3 \
    "$@"
