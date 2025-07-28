#!/usr/bin/env bash

# Sets/updates the given kv-pair in the given properties file.
#
# @param $1 The properties file.
# @param $2 The key to set.
# @param $3 The value to set.
update_config_file() {
    local file_path=$1
    local key=$2
    local value=$3

    if grep --quiet "^${key}=.*$" "$file_path"; then
        sed --in-place "s|^${key}=.*|${key}=${value}|" "$file_path"
    else
        echo "${key}=${value}" >> "$file_path"
    fi
    echo "Set ${key}=${value} in ${file_path}"
}

apt-get update && apt-get install --assume-yes --no-install-recommends jq wget

PRESTO_CONFIG_DIR="/opt/presto-server/etc"

# Substitute environemnt variables in config template
find /configs -type f | while read -r f; do
    (
        echo "cat <<EOF"
        cat "$f"
        echo "EOF"
    ) | sh >"${PRESTO_CONFIG_DIR}/$(basename "$f")"
done

# Setup the config directory hierarchy
rm -f ${PRESTO_CONFIG_DIR}/catalog/*

mv ${PRESTO_CONFIG_DIR}/clp.properties ${PRESTO_CONFIG_DIR}/catalog

# Update "presto.version" parameter in config.properties file using values from coordinator
CONFIG_PROPERTIES_FILE="/opt/presto-server/etc/config.properties"

# 1. Fetch version info from Presto
DISCOVERY_URI=$(awk -F= '/^discovery.uri=/ {print $2}' "${PRESTO_CONFIG_DIR}/config.properties")
if response=$(
    wget --quiet --output-document - --timeout=10 "${DISCOVERY_URI}/v1/info" 2>/dev/null
); then
    version=$(echo "$response" | jq --raw-output '.nodeVersion.version')
    if [ "$version" != "null" ]; then
        echo "Presto is ready!"
    else
        echo "Error: Presto response is empty or doesn't contain version info."
        exit 1
    fi
else
    echo "Error: Couldn't get Presto version info."
    exit 1
fi

# 2. Extract the version using grep and sed (busybox compatible)
version=$(echo "$response" | grep -o '"version":"[^"]*"' | sed 's/"version":"//;s/"//')

echo "Detected Presto version: $version"

update_config_file "$CONFIG_PROPERTIES_FILE" "presto.version" "$version"

# Update node.properties
NODE_PROPERTIES_FILE="/opt/presto-server/etc/node.properties"
update_config_file "$NODE_PROPERTIES_FILE" "node.internal-address" "$(hostname -i)"
update_config_file "$NODE_PROPERTIES_FILE" "node.id" "$(hostname)"
