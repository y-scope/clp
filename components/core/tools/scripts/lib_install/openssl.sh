#!/usr/bin/env bash

# Dependencies: a C compiler, curl, make, Perl, and tar.
set -o errexit
set -o nounset
set -o pipefail

if [[ "$#" -lt 1 || "$#" -gt 3 ]]; then
    echo "Usage: ${BASH_SOURCE[0]} <version> [prefix] [openssldir]" >&2
    exit 1
fi
version="$1"
prefix="${2:-/usr/local}"
openssldir="${3:-${prefix}/ssl}"

if [[ ! "${version}" =~ ^[0-9]+\.[0-9]+\.[0-9]+[a-z]?$ ]]; then
    echo "Invalid OpenSSL version: ${version}" >&2
    exit 1
fi
if [[ "${prefix}" != /* || "${openssldir}" != /* ]]; then
    echo "The installation prefix and OpenSSL directory must be absolute paths." >&2
    exit 1
fi

install_cmd_args=()
if [[ ${EUID} -ne 0 ]]; then
    install_cmd_args+=("sudo")
fi

temp_dir="$(mktemp -d)"
trap 'rm -rf "${temp_dir}"' EXIT
package_name="openssl-${version}"
cd "${temp_dir}"
curl -fsSL "https://www.openssl.org/source/${package_name}.tar.gz" -o "${package_name}.tar.gz"
tar -xf "${package_name}.tar.gz"
cd "${package_name}"

./config --prefix="${prefix}" --openssldir="${openssldir}"
make -j"$(nproc)"
# install_sw skips documentation. Install the configuration as well, into this version's prefix.
"${install_cmd_args[@]}" make install_sw install_ssldirs
