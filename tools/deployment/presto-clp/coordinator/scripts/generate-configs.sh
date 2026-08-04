#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

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
