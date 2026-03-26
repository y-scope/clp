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
#
# CA bundle path used by tools (via env vars in the Dockerfile):
#   /opt/corp-ca/ca-bundle.crt
#
# This path is outside the system package manager's control, so it survives
# reinstalls of ca-certificates/ca-trust packages that regenerate the system
# bundle. The Dockerfile points SSL_CERT_FILE, CURL_CA_BUNDLE, REQUESTS_CA_BUNDLE,
# and PIP_CERT at this path.

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
ca_cert="${script_dir}/ca-certificates.crt"

if [[ ! -s "$ca_cert" ]]; then
    # No corporate proxy — create the stable bundle from the system certs so
    # that env vars (PIP_CERT, CURL_CA_BUNDLE, etc.) pointing to it are valid.
    echo "corporate-proxy-container: no corporate CA bundle provided; using system certs."
    mkdir -p /opt/corp-ca
    cp /etc/ssl/certs/ca-certificates.crt /opt/corp-ca/ca-bundle.crt 2>/dev/null \
        || cp /etc/pki/tls/certs/ca-bundle.crt /opt/corp-ca/ca-bundle.crt 2>/dev/null \
        || touch /opt/corp-ca/ca-bundle.crt
    exit 0
fi

echo "corporate-proxy-container: installing corporate CA certificates..."

# Save to a stable path outside the system package manager's control.
# Tools are pointed here via env vars (SSL_CERT_FILE, PIP_CERT, etc.) so the
# cert survives later reinstalls of ca-certificates/ca-trust packages.
mkdir -p /opt/corp-ca
cp "$ca_cert" /opt/corp-ca/ca-bundle.crt

if [[ -d /etc/pki/tls/certs ]]; then
    # RHEL/CentOS/manylinux: also copy to the system bundle for tools that
    # don't use env vars.
    cp "$ca_cert" /etc/pki/tls/certs/ca-bundle.crt
    echo "corporate-proxy-container: installed CA bundle (RHEL/manylinux)."
else
    # Debian/Ubuntu/Alpine: also copy to the system bundle for tools that
    # don't use env vars.
    mkdir -p /etc/ssl/certs
    cp "$ca_cert" /etc/ssl/certs/ca-certificates.crt
    echo "corporate-proxy-container: installed CA bundle (Debian/Alpine)."
fi

# Remove the staging file — it's no longer needed after installation into the
# trust store.
rm "$ca_cert"
