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
export CLP_HOST_ARCH
CLP_HOST_ARCH="$(uname -m)"

# --- First-run telemetry consent prompt ---
# Runs in a short-lived container with --tty so stdin is a real TTY. The helper
# checks env vars (CLP_DISABLE_TELEMETRY, DO_NOT_TRACK), the config file, and
# whether this is a first run (no instance-id). If needed, it prompts the user
# and writes the consent choice to the config file.
if [[ -t 0 ]]; then
    docker compose -f "$CLP_HOME/docker-compose.runtime.yaml" \
        run --rm --tty --interactive --no-deps \
        -e CLP_DISABLE_TELEMETRY="${CLP_DISABLE_TELEMETRY:-}" \
        -e DO_NOT_TRACK="${DO_NOT_TRACK:-}" \
        clp-runtime \
        python3 \
        -m clp_package_utils.scripts.check_telemetry_consent \
        --config "$CLP_HOME/etc/clp-config.yaml" \
        --clp-home "$CLP_HOME"
fi

# --- Pass telemetry opt-out vars to the main container ---
# start_clp.py reads these to persist the consent choice to the config file
# and to set the correct telemetry state for all service containers.
_telemetry_env_flags=()
if [[ -n "${CLP_DISABLE_TELEMETRY:-}" ]]; then
    _telemetry_env_flags+=(-e CLP_DISABLE_TELEMETRY="$CLP_DISABLE_TELEMETRY")
fi
if [[ -n "${DO_NOT_TRACK:-}" ]]; then
    _telemetry_env_flags+=(-e DO_NOT_TRACK="$DO_NOT_TRACK")
fi

docker compose -f "$CLP_HOME/docker-compose.runtime.yaml" \
    run --rm "${CLP_COMPOSE_RUN_EXTRA_FLAGS[@]}" "${_telemetry_env_flags[@]}" clp-runtime \
    python3 \
    -m clp_package_utils.scripts.start_clp \
    "$@"
