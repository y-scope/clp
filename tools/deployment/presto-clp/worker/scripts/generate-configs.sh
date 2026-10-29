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
    if response=$(
        wget --quiet --output-document - --timeout 10 "${discovery_uri}/v1/info" 2>/dev/null
    ); then
        version=$(echo "$response" | jq --raw-output '.nodeVersion.version')
        if [[ "$version" = "null" ]]; then
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

apt-get update && apt-get install --assume-yes --no-install-recommends jq wget

readonly PRESTO_CONFIG_DIR="/opt/presto-server/etc"

# Substitute environment variables in config template
find /configs -type f | while read -r f; do
    (
        echo "cat <<EOF"
        cat "$f"
        echo "EOF"
    ) | sh >"${PRESTO_CONFIG_DIR}/$(basename "$f")"
done

# Remove existing catalog files that exist in the image and add the CLP catalog
rm -f "${PRESTO_CONFIG_DIR}/catalog/"*
mv "${PRESTO_CONFIG_DIR}/clp.properties" "${PRESTO_CONFIG_DIR}/catalog"

# Update config.properties
readonly CONFIG_PROPERTIES_FILE="/opt/presto-server/etc/config.properties"
version=$(get_coordinator_version "$CONFIG_PROPERTIES_FILE")
log "INFO" "Detected Presto version: $version"
update_config_file "$CONFIG_PROPERTIES_FILE" "presto.version" "$version"

# Update node.properties
readonly NODE_PROPERTIES_FILE="/opt/presto-server/etc/node.properties"
update_config_file "$NODE_PROPERTIES_FILE" "node.internal-address" "$(hostname -i)"
update_config_file "$NODE_PROPERTIES_FILE" "node.id" "$(hostname)"
