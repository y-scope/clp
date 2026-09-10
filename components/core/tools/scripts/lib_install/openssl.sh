#!/usr/bin/env bash

# Exit on error
set -e

cUsage="Usage: ${BASH_SOURCE[0]} <version> [prefix] [openssldir]"
if [ "$#" -lt 1 ] ; then
    echo $cUsage
    exit
fi
version="$1"
prefix="${2:-/opt/openssl-${version}}"
openssldir="${3:-$prefix/ssl}"  # Default to prefix/ssl if not specified

package_name=openssl-${version}
temp_dir="/tmp/${package_name}-installation"
mkdir -p "$temp_dir"

echo "🔧 OpenSSL version: $version"
echo "📁 Install prefix: $prefix"
echo "📂 OpenSSL directory: $openssldir"

echo "Checking for elevated privileges..."
if [ ${EUID:-$(id -u)} -ne 0 ] ; then
  sudo echo "Script can elevate privileges."
fi

# Download and extract
cd "${temp_dir}"
curl -LO "https://www.openssl.org/source/openssl-${version}.tar.gz"
tar -xf "openssl-${version}.tar.gz"
cd "openssl-${version}"

# Configure and build
./config --prefix="$prefix" --openssldir="$openssldir" -fPIC
make -j"$(nproc)"
make install_sw  # install_sw skips man pages

echo "✅ OpenSSL $version installed to $prefix"
echo "📄 Runtime config expected at $openssldir"

# Clean up
rm -rf "$temp_dir"
