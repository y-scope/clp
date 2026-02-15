#!/usr/bin/env bash

# Creates an Alpine .apk package from pre-built CLP core binaries.
#
# This script runs INSIDE the apk-builder container as root. It calls
# bundle-libs.sh to collect and patch binaries, then uses abuild to create
# a properly formatted .apk package.
#
# The resulting package is built on musllinux_1_2 (musl libc), making it
# compatible with Alpine Linux 3.20+.
#
# Required environment variables:
#   PKG_VERSION  Package version (e.g., "0.9.1" or "0.9.1-20260214.5f1d7ca")
#   PKG_ARCH     Target APK architecture: x86_64 or aarch64
#   BIN_DIR      Path to the directory containing compiled binaries
#
# Optional environment variables:
#   OUTPUT_DIR   Where to write the .apk file (default: current directory)

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

# --- Validate inputs --------------------------------------------------------

if [[ -z "${PKG_VERSION:-}" ]]; then
    echo "ERROR: PKG_VERSION is required" >&2
    exit 1
fi

if [[ -z "${PKG_ARCH:-}" ]]; then
    echo "ERROR: PKG_ARCH is required" >&2
    exit 1
fi

if [[ -z "${BIN_DIR:-}" ]]; then
    echo "ERROR: BIN_DIR is required" >&2
    exit 1
fi

output_dir="${OUTPUT_DIR:-$(pwd)}"
staging="/tmp/clp-apk-staging"

# APK version: hyphens are invalid and underscore suffixes must use a recognized
# tag (_alpha, _beta, _pre, _rc, _git, etc.) followed by digits only. We use
# _git with the date; the commit hash is stored in pkgdesc since apk versions
# don't allow hex characters.
# E.g., "0.9.1-20260214.5f1d7ca" -> pkgver=0.9.1_git20260214, hash in pkgdesc
apk_desc="CLP core universal binaries for log compression and search"
if [[ "${PKG_VERSION}" == *-* ]]; then
    base_version="${PKG_VERSION%%-*}"
    snapshot_suffix="${PKG_VERSION#*-}"
    snapshot_date="${snapshot_suffix%%.*}"
    snapshot_hash="${snapshot_suffix#*.}"
    apk_version="${base_version}_git${snapshot_date}"
    apk_desc="${apk_desc} (commit: ${snapshot_hash})"
else
    apk_version="${PKG_VERSION}"
fi

# --- Bundle binaries and libraries ------------------------------------------

STAGING_DIR="${staging}" \
BIN_DIR="${BIN_DIR}" \
    "${script_dir}/../common/bundle-libs.sh"

# --- Build .apk via abuild -------------------------------------------------

abuild_dir="/tmp/clp-abuild"
rm -rf "${abuild_dir}"
mkdir -p "${abuild_dir}"

# abuild requires a signing key; add it to trusted keys so the post-build
# repository index step succeeds.
abuild-keygen -an 2>/dev/null || true
cp /root/.abuild/*.rsa.pub /etc/apk/keys/ 2>/dev/null || true

# Create APKBUILD that copies our pre-bundled staging directory
cat > "${abuild_dir}/APKBUILD" <<APKBUILD
# Maintainer: YScope Inc. <support@yscope.com>
pkgname=clp-core
pkgver=${apk_version}
pkgrel=0
pkgdesc="${apk_desc}"
url="https://github.com/y-scope/clp"
arch="${PKG_ARCH}"
license="Apache-2.0"
depends="musl libstdc++"
source=""
options="!check !strip"

package() {
    mkdir -p "\${pkgdir}/usr"
    cp -a "${staging}"/* "\${pkgdir}"/
}
APKBUILD

echo "==> Building apk package..."

cd "${abuild_dir}"
# -F: force (allows running as root, skips fakeroot)
# -d: disable dependency checking
# -P: set PKGDEST (where the .apk is written)
abuild -F checksum
abuild -F -d -P "/tmp/clp-apk-out"

# Copy the built package to the output directory
if ! find /tmp/clp-apk-out -name "*.apk" | grep --quiet .; then
    echo "ERROR: abuild produced no .apk files" >&2
    exit 1
fi
find /tmp/clp-apk-out -name "*.apk" -exec cp {} "${output_dir}/" \;
