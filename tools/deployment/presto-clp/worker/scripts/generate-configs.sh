#!/bin/sh

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
if response=$(wget -qO- --timeout=10 "${DISCOVERY_URI}/v1/info" 2>/dev/null); then
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

# 3. Replace `presto.version=REPLACE_ME` with actual version in the config file
if grep -q '^presto.version=REPLACE_ME' "$CONFIG_PROPERTIES_FILE"; then
    sed -i "s|^presto.version=REPLACE_ME|presto.version=$version|" "$CONFIG_PROPERTIES_FILE"
    echo "Updated $CONFIG_PROPERTIES_FILE with version $version"
else
    echo "Warning: 'presto.version=REPLACE_ME' not found in $CONFIG_PROPERTIES_FILE"
    exit 1
fi

# Modify node.properties
NODE_PROPERTIES_FILE="/opt/presto-server/etc/node.properties"
INTERNAL_ADDRESS=$(hostname -i)
# Replace `node.internal-address=REPLACE_ME` with actual ip address in the config file
if grep -q '^node.internal-address=REPLACE_ME' "$NODE_PROPERTIES_FILE"; then
    sed -i \
        "s|^node.internal-address=REPLACE_ME|node.internal-address=${INTERNAL_ADDRESS}|" \
        "$NODE_PROPERTIES_FILE"
    echo "Updated $NODE_PROPERTIES_FILE with node.internal-address ${INTERNAL_ADDRESS}"
else
    echo "Warning: 'node.internal-address=REPLACE_ME' not found in $NODE_PROPERTIES_FILE"
    exit 1
fi

# Replace `node.id=REPLACE_ME` with actual hostname in the config file
if grep -q '^node.id=REPLACE_ME' "$NODE_PROPERTIES_FILE"; then
    sed -i "s|^node.id=REPLACE_ME|node.id=$(hostname)|" "$NODE_PROPERTIES_FILE"
    echo "Updated $NODE_PROPERTIES_FILE with node.id $(hostname)"
else
    echo "Warning: 'node.id=REPLACE_ME' not found in $NODE_PROPERTIES_FILE"
    exit 1
fi
