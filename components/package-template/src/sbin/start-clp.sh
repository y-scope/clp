#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
common_env_path="$script_dir/.common-env.sh"

# shellcheck source=.common-env.sh
source "$common_env_path"

# --- Export host OS info for telemetry ---
# These environment variables are passed to containers for OpenTelemetry resource attributes.
CLP_HOST_OS="$(uname -s | tr '[:upper:]' '[:lower:]')"
export CLP_HOST_OS
if [[ -f /etc/os-release ]]; then
    CLP_HOST_OS_VERSION="$(. /etc/os-release && echo "${ID:-unknown}-${VERSION_ID:-unknown}")"
else
    CLP_HOST_OS_VERSION="unknown"
fi
export CLP_HOST_OS_VERSION
CLP_HOST_ARCH="$(uname -m)"
export CLP_HOST_ARCH

docker compose -f "$CLP_HOME/docker-compose.runtime.yaml" \
    run --rm "${CLP_COMPOSE_RUN_EXTRA_FLAGS[@]}" clp-runtime \
    python3 \
    -m clp_package_utils.scripts.start_clp \
    "$@"
