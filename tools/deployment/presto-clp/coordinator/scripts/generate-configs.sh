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

# Remove existing catalog files that exist in the image and add the CLP catalog
rm -f "${PRESTO_CONFIG_DIR}/catalog/"*
mv "${PRESTO_CONFIG_DIR}/clp.properties" "${PRESTO_CONFIG_DIR}/catalog"
