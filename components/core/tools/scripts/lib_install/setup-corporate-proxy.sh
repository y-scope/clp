#!/usr/bin/env bash

# Installs a corporate CA certificate bundle into the container's system trust
# store. This is needed when building behind a TLS-intercepting proxy — without
# the corporate CA in the trust store, tools like curl, dnf, and pip will reject
# the proxy's certificates with SSL verification errors.
#
# The CA bundle (ca-certificates.crt) is placed alongside this script by the
# host-side build.sh (via proxy-lib.sh). If the file is empty, no corporate
# proxy is in use and this script is a no-op.
#
# Supports:
#   - DNF-based (manylinux_2_28, centos-stream-9): update-ca-trust
#   - APK/APT-based (musllinux_1_2, ubuntu-jammy): update-ca-certificates

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
ca_cert="${script_dir}/ca-certificates.crt"

if [[ ! -s "$ca_cert" ]]; then
    echo "setup-corporate-proxy: no CA bundle provided, skipping."
    exit 0
fi

echo "setup-corporate-proxy: installing corporate CA certificates..."

if command -v update-ca-trust &>/dev/null; then
    # DNF-based distros (RHEL, CentOS, manylinux_2_28)
    cp "$ca_cert" /etc/pki/ca-trust/source/anchors/corporate-ca-bundle.crt
    update-ca-trust extract
    echo "setup-corporate-proxy: updated trust store via update-ca-trust."
elif command -v update-ca-certificates &>/dev/null; then
    # APK/APT-based distros (Alpine, Ubuntu)
    cp "$ca_cert" /usr/local/share/ca-certificates/corporate-ca-bundle.crt
    update-ca-certificates
    echo "setup-corporate-proxy: updated trust store via update-ca-certificates."
else
    echo "setup-corporate-proxy: WARNING: no recognized CA trust update tool found." >&2
    exit 1
fi
