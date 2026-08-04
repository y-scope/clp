#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

# Emits a log event to stderr with an auto-generated ISO timestamp as well as the given level
# and message.
#
# @param $1: Level string
# @param $2: Message to be logged
log() {
    local -r LEVEL=$1
    local -r MESSAGE=$2
    echo "$(date --utc --date="now" +"%Y-%m-%dT%H:%M:%SZ") [${LEVEL}] ${MESSAGE}" >&2
}

# Gets the Presto coordinator's version or exits on failure.
#
# @param $1 Path to the config.properties file.
# @return The Presto version.
get_coordinator_version() {
    local config_properties_file=$1

    local discovery_uri
    discovery_uri=$(awk -F "=" '/^discovery.uri=/ {print $2}' "$config_properties_file")
    if response=$(curl --fail --silent --max-time 10 "${discovery_uri}/v1/info"); then
        if ! version=$(
            echo "$response" \
                | python3 -c \
                    "import json, sys; print(json.load(sys.stdin)['nodeVersion']['version'])" \
                    2>/dev/null
        ); then
            log "ERROR" "Presto response is empty or doesn't contain version info."
            exit 1
        fi
    else
        log "ERROR" "Couldn't get Presto version info."
        exit 1
    fi

    echo "$version"
}

# Sets/updates the given kv-pair in the given properties file.
#
# @param $1 Path to the properties file.
# @param $2 The key to set.
# @param $3 The value to set.
update_config_file() {
    local file_path=$1
    local key=$2
    local value=$3

    if grep --quiet "^${key}=.*$" "$file_path"; then
        sed --in-place "s|^${key}=.*|${key}=${value}|" "$file_path"
    else
        echo "${key}=${value}" >>"$file_path"
    fi
    log "INFO" "Set ${key}=${value} in ${file_path}"
}

readonly PRESTO_CONFIG_DIR="/opt/presto-server/etc"

# Substitute environment variables in config template
find /configs -type f | while read -r f; do
    (
        echo "cat <<EOF"
        cat "$f"
        echo "EOF"
    ) | sh >"${PRESTO_CONFIG_DIR}/$(basename "$f")"
done

# Create the catalog directory and add the CLP catalog
mkdir -p "${PRESTO_CONFIG_DIR}/catalog"
mv "${PRESTO_CONFIG_DIR}/clp.properties" "${PRESTO_CONFIG_DIR}/catalog"

# Update config.properties
readonly CONFIG_PROPERTIES_FILE="/opt/presto-server/etc/config.properties"
version=$(get_coordinator_version "$CONFIG_PROPERTIES_FILE")
log "INFO" "Detected Presto version: $version"
update_config_file "$CONFIG_PROPERTIES_FILE" "presto.version" "$version"

# Update node.properties
#
# NOTE: These are resolved through Python rather than `hostname`, which the Presto worker image
# doesn't ship. Assigning them first ensures a resolution failure aborts the script instead of
# silently writing empty values.
readonly NODE_PROPERTIES_FILE="/opt/presto-server/etc/node.properties"
node_internal_address=$(python3 -c "import socket; print(socket.gethostbyname(socket.gethostname()))")
node_id=$(python3 -c "import socket; print(socket.gethostname())")
update_config_file "$NODE_PROPERTIES_FILE" "node.internal-address" "$node_internal_address"
update_config_file "$NODE_PROPERTIES_FILE" "node.id" "$node_id"
