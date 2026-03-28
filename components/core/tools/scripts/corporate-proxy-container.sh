#!/usr/bin/env bash

# CONTAINER-SIDE half of the two-part corporate proxy support system for Docker image builds.
# Counterpart: tools/scripts/corporate-proxy-host.sh (runs on the host before `docker build`).
#
# Two execution paths:
#
#   LOCAL BUILD (behind corporate proxy):
#     build.sh sources corporate-proxy-host.sh, which:
#       1. Detects the host's real CA bundle (with corporate certs from Zscaler/Fortinet/etc.)
#       2. Copies it into the Docker build context as ca-certificates.crt
#       3. Forwards proxy env vars (HTTP_PROXY, HTTPS_PROXY) as --build-arg
#       4. Auto-switches to --network host if the proxy is on localhost
#       5. Passes mirror URL overrides (APT_MIRROR_URL/DNF_MIRROR_BASE_URL)
#     Then this script installs the real CA bundle into /opt/corp-ca/ca-bundle.crt.
#
#   CI / NON-PROXY BUILD:
#     The CI workflow copies the runner's system CA bundle into the build context
#     as ca-certificates.crt before running docker build. This script then installs
#     it into /opt/corp-ca/ca-bundle.crt just like the corporate proxy case.
#
# In both paths, ca-certificates.crt must be non-empty. An empty or missing file
# is an error — it means the build was invoked without proper CA setup.
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
    echo "ERROR: ca-certificates.crt is empty or missing." >&2
    echo "  Local builds: run build.sh (sources corporate-proxy-host.sh)." >&2
    echo "  CI builds: ensure the workflow stages a CA bundle." >&2
    exit 1
fi

echo "corporate-proxy-container: installing CA certificates..."

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
    mkdir -p "/etc/ssl/certs"
    cp "$ca_cert" /etc/ssl/certs/ca-certificates.crt
    echo "corporate-proxy-container: installed CA bundle (Debian/Alpine)."
fi

# Remove the staging file — it's no longer needed after installation into the
# trust store.
rm "$ca_cert"
