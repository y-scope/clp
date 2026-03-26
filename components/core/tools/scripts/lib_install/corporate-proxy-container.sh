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

if [[ -d /etc/pki/tls/certs ]]; then
    # RHEL/CentOS/manylinux: direct copy for immediate effect; also register in
    # the trust anchors dir so update-ca-trust (called by rpm post-install
    # scripts) re-includes the cert on package reinstalls.
    cp "$ca_cert" /etc/pki/tls/certs/ca-bundle.crt
    mkdir -p /etc/pki/ca-trust/source/anchors
    cp "$ca_cert" /etc/pki/ca-trust/source/anchors/corporate-proxy.crt
    echo "corporate-proxy-container: installed CA bundle (RHEL/manylinux)."
else
    # Debian/Ubuntu/Alpine: direct copy for immediate effect; also register in
    # /usr/local/share/ca-certificates/ so update-ca-certificates (called by
    # apt post-install scripts) re-includes the cert on package reinstalls.
    # We deliberately do NOT call update-ca-certificates ourselves to avoid
    # a bootstrap dependency on that tool being installed.
    mkdir -p /etc/ssl/certs /usr/local/share/ca-certificates
    cp "$ca_cert" /etc/ssl/certs/ca-certificates.crt
    cp "$ca_cert" /usr/local/share/ca-certificates/corporate-proxy.crt
    echo "corporate-proxy-container: installed CA bundle (Debian/Alpine)."
fi

# Remove the staging file — it's no longer needed after installation into the
# trust store.
rm "$ca_cert"
