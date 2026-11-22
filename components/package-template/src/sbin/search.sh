#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
common_env_path="$script_dir/.common-env.sh"

# shellcheck source=.common-env.sh
source "$common_env_path"

# Once stdout is closed (e.g., by `| head`), `docker compose run` exits, but the container keeps
# running the search, so the container is removed when this script exits.
container_name="clp-search-$$"
trap 'docker rm --force "$container_name" &>/dev/null || true' EXIT

docker compose -f "$CLP_HOME/docker-compose.runtime.yaml" \
    run --rm --name "$container_name" "${CLP_COMPOSE_RUN_EXTRA_FLAGS[@]}" clp-runtime \
    /opt/clp/bin/clp-search \
    "$@"
