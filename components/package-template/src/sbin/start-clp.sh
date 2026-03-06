#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
common_env_path="$script_dir/.common-env.sh"

# shellcheck source=.common-env.sh
source "$common_env_path"

# --- Telemetry consent prompt ---
# Determines whether to show the first-run telemetry consent prompt.
# The prompt is skipped if:
#   1. CLP_DISABLE_TELEMETRY or DO_NOT_TRACK env vars are set
#   2. telemetry.disable is explicitly set in clp-config.yaml
#   3. instance-id file already exists (not a first run)
# If shown and the user declines, telemetry.disable is written to clp-config.yaml.

telemetry_prompt_needed=false

# Find the config file path (mirror the default in start_clp.py)
clp_config_path="${CLP_HOME}/etc/clp-config.yaml"

# Check env vars first
clp_disable_telemetry_lower="${CLP_DISABLE_TELEMETRY:-}"
clp_disable_telemetry_lower="${clp_disable_telemetry_lower,,}"
if [[ "$clp_disable_telemetry_lower" == "true" ]] || [[ "$clp_disable_telemetry_lower" == "1" ]]; then
    telemetry_prompt_needed=false
elif [[ "${DO_NOT_TRACK:-}" == "1" ]]; then
    telemetry_prompt_needed=false
# Check if telemetry is already configured in the config file
elif [[ -f "$clp_config_path" ]] && grep -q "telemetry:" "$clp_config_path" 2>/dev/null; then
    telemetry_prompt_needed=false
# Check if instance-id exists (not first run)
elif [[ -f "${CLP_HOME}/var/log/instance-id" ]]; then
    telemetry_prompt_needed=false
else
    telemetry_prompt_needed=true
fi

if [[ "$telemetry_prompt_needed" == "true" ]]; then
    if [[ -t 0 ]]; then
        # Interactive: show the consent prompt
        echo "================================================================================"
        echo "CLP collects anonymous usage telemetry to help improve the software."
        echo "This includes: CLP version, OS/architecture, deployment method, and"
        echo "component health status. It does NOT include: log content, queries,"
        echo "hostnames, IP addresses, or any personally identifiable"
        echo "information."
        echo ""
        echo "Telemetry is sent to: https://telemetry.yscope.io"
        echo "For details, see: https://docs.yscope.com/clp/main/user-guide/telemetry"
        echo ""
        echo "You can disable telemetry at any time by setting CLP_DISABLE_TELEMETRY=true"
        echo "or by blocking https://telemetry.yscope.io at the network level."
        echo ""
        read -r -p "Enable anonymous telemetry to help improve CLP? [Y/n] " telemetry_response
        echo "================================================================================"

        if [[ "$telemetry_response" =~ ^[Nn]$ ]]; then
            # User opted out — persist to config
            if [[ -f "$clp_config_path" ]]; then
                echo "" >> "$clp_config_path"
                echo "telemetry:" >> "$clp_config_path"
                echo "  disable: true" >> "$clp_config_path"
            else
                printf "telemetry:\n  disable: true\n" > "$clp_config_path"
            fi
            echo "Telemetry has been disabled. You can re-enable it in ${clp_config_path}."
        fi
    fi
    # Non-interactive: default to enabled (no prompt, no config write needed)
fi

# --- Export host OS info for telemetry ---
export CLP_HOST_OS="linux"
if [[ -f /etc/os-release ]]; then
    CLP_HOST_OS_VERSION="$(. /etc/os-release && echo "${ID:-unknown}-${VERSION_ID:-unknown}")"
else
    CLP_HOST_OS_VERSION="unknown"
fi
export CLP_HOST_OS_VERSION
export CLP_HOST_ARCH="$(uname -m)"

docker compose -f "$CLP_HOME/docker-compose.runtime.yaml" \
    run --rm "${CLP_COMPOSE_RUN_EXTRA_FLAGS[@]}" clp-runtime \
    python3 \
    -m clp_package_utils.scripts.start_clp \
    "$@"

