#!/bin/sh

# Exit on error
set -e

PRESTO_CONFIG_DIR="/opt/presto-server/etc"

# Substitute environemnt variables in config template
find /configs -type f | while read -r f; do
  ( echo "cat <<EOF"; cat $f; echo "EOF" ) | sh > "${PRESTO_CONFIG_DIR}/$(basename "$f")"
done

# Setup the config directory hierarchy
rm -f ${PRESTO_CONFIG_DIR}/catalog/*

# Copy over files
mv ${PRESTO_CONFIG_DIR}/clp.properties ${PRESTO_CONFIG_DIR}/catalog

