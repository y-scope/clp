#!/usr/bin/env bash

# Installs a corporate CA certificate bundle into the container's system trust store. This is needed
# when building behind a TLS-intercepting proxy — without the corporate CA in the trust store, tools
# like curl, dnf, and pip will reject the proxy's certificates with SSL verification errors.
#
# The CA bundle (ca-certificates.crt) is placed alongside this script by the host-side build.sh (via
# proxy-lib.sh). If the file is empty, no corporate proxy is in use and this script is a no-op.
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
echo "setup-corporate-proxy: installed CA bundle to ${system_cert}."

# Remove the staging file — it's no longer needed after installation into the
# trust store.
rm "$ca_cert"
