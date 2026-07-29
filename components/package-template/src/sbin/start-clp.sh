#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
common_env_path="$script_dir/.common-env.sh"

# shellcheck source=.common-env.sh
source "$common_env_path"

docker compose -f "$CLP_HOME/docker-compose.runtime.yaml" \
    run --rm "${CLP_COMPOSE_RUN_EXTRA_FLAGS[@]}" clp-runtime \
    python3 \
    -m clp_package_utils.scripts.start_clp \
    "$@"
