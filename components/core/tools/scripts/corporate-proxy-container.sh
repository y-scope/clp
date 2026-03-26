#!/usr/bin/env bash

# CONTAINER-SIDE half of the two-part corporate proxy support system for Docker image builds.
# Counterpart: tools/scripts/corporate-proxy-host.sh (runs on the host before `docker build`).
#
# Two execution paths:
#
#   LOCAL BUILD (behind corporate proxy):
#     build.sh sources corporate-proxy-host.sh, which:
#       1. Detects the host's real CA bundle (with corporate certs from Zscaler/Fortinet/etc.)
#       2. Copies it into the Docker build context, overwriting the placeholder ca-certificates.crt
#       3. Forwards proxy env vars (HTTP_PROXY, HTTPS_PROXY) as --build-arg
#       4. Auto-switches to --network host if the proxy is on localhost
#       5. Passes mirror URL overrides (APT_MIRROR_URL/DNF_MIRROR_BASE_URL)
#     Then this script installs the real CA bundle into /opt/corp-ca/ca-bundle.crt.
#
#   CI / NON-PROXY BUILD:
#     docker build runs directly without corporate-proxy-host.sh. The committed empty
#     ca-certificates.crt placeholder is COPYed in. This script detects the empty file and
#     creates /opt/corp-ca/ca-bundle.crt from system certs (or as an empty file on bare images
#     like ubuntu:jammy where system certs aren't installed yet). The empty file is needed because
#     Dockerfile ENV vars (PIP_CERT, CURL_CA_BUNDLE, etc.) point to this path, and pip hard-errors
#     on a missing file but works fine with an empty one.
#
# If the staged ca-certificates.crt file is empty, no corporate proxy is in use — the script
# copies system certs to /opt/corp-ca/ca-bundle.crt (or creates an empty file on bare images).
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

corp_ca_dir="/opt/corp-ca"
corp_ca_bundle="${corp_ca_dir}/ca-bundle.crt"
mkdir -p "${corp_ca_dir}"

if [[ ! -s "$ca_cert" ]]; then
    # No corporate proxy — copy the system certs to the stable path so that env
    # vars (PIP_CERT, CURL_CA_BUNDLE, etc.) pointing to it are valid.
    if [[ -f /etc/ssl/certs/ca-certificates.crt ]]; then
        echo "corporate-proxy-container: no corporate CA; copying system certs to ${corp_ca_dir}/."
        cp /etc/ssl/certs/ca-certificates.crt "${corp_ca_bundle}"
    elif [[ -f /etc/pki/tls/certs/ca-bundle.crt ]]; then
        echo "corporate-proxy-container: no corporate CA; copying system certs to ${corp_ca_dir}/."
        cp /etc/pki/tls/certs/ca-bundle.crt "${corp_ca_bundle}"
    else
        # No system certs yet (e.g., bare ubuntu:jammy before ca-certificates is installed).
        # Create an empty bundle so Dockerfile ENV vars (PIP_CERT, CURL_CA_BUNDLE, etc.)
        # point to a valid file. pip hard-errors on a missing path but works with an empty file.
        echo "corporate-proxy-container: no corporate CA and no system certs yet; creating empty bundle."
        touch "${corp_ca_bundle}"
    fi
    exit 0
fi

echo "corporate-proxy-container: installing corporate CA certificates..."

# Save to a stable path outside the system package manager's control.
# Tools are pointed here via env vars (SSL_CERT_FILE, PIP_CERT, etc.) so the
# cert survives later reinstalls of ca-certificates/ca-trust packages.
cp "$ca_cert" "${corp_ca_bundle}"

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
