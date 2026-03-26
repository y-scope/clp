#!/usr/bin/env bash

# CONTAINER-SIDE half of the two-part corporate proxy support system for Docker image builds.
# Counterpart: tools/scripts/corporate-proxy-host.sh (runs on the host before `docker build`).
#
# Two-part flow:
#   1. (Host)      corporate-proxy-host.sh is sourced by each docker-images/*/build.sh. It detects
#                  the host's CA bundle, stages it into the build context as ca-certificates.crt,
#                  and injects proxy env vars as Docker --build-arg flags.
#   2. (Container) This script runs as a Dockerfile RUN step. It installs the staged CA bundle into
#                  the container's system trust store so that tools like curl, dnf, and pip can
#                  reach the internet through a TLS-intercepting proxy.
#
# If the staged ca-certificates.crt file is empty, no corporate proxy is in use and this script
# is a no-op.
#
# Supports:
#   - DNF-based (manylinux_2_28, centos-stream-9)
#   - APK/APT-based (musllinux_1_2, ubuntu-jammy)

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
ca_cert="${script_dir}/ca-certificates.crt"

if [[ ! -s "$ca_cert" ]]; then
    echo "corporate-proxy-container: no CA bundle provided, skipping."
    exit 0
fi

echo "corporate-proxy-container: installing corporate CA certificates..."

# Detect the system certificate bundle path.
if [[ -d /etc/pki/tls/certs ]]; then
    # RHEL/CentOS/manylinux
    system_cert="/etc/pki/tls/certs/ca-bundle.crt"
elif [[ -d /etc/ssl/certs ]]; then
    # Debian/Ubuntu/Alpine
    system_cert="/etc/ssl/certs/ca-certificates.crt"
else
    mkdir -p /etc/ssl/certs
    system_cert="/etc/ssl/certs/ca-certificates.crt"
fi

# The source bundle from the host is already a combined trust store.
# Copy it directly — no update-ca-trust or update-ca-certificates needed.
cp "$ca_cert" "$system_cert"
echo "corporate-proxy-container: installed CA bundle to ${system_cert}."

# Remove the staging file — it's no longer needed after installation into the
# trust store.
rm "$ca_cert"
