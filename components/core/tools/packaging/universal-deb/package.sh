#!/usr/bin/env bash

# Creates a universal .deb package from pre-built CLP core binaries.
#
# This script runs INSIDE the deb-builder container. It calls bundle-libs.sh
# to collect and patch binaries, then builds a .deb with dpkg-deb.
#
# The resulting package is "universal" — built on manylinux_2_28 (glibc >= 2.28),
# it's compatible with Debian 10+, Ubuntu 20.04+, and other glibc-based Debian
# derivatives.
#
# Required environment variables:
#   PKG_VERSION  Package version (e.g., "0.9.1" or "0.9.1-20260214.5f1d7ca")
#   PKG_ARCH     Target package architecture: amd64 or arm64
#   BIN_DIR      Path to the directory containing compiled binaries
#
# Optional environment variables:
#   OUTPUT_DIR   Where to write the .deb file (default: current directory)

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
staging="/tmp/clp-deb-staging"

# Deb version: replace hyphen with tilde so pre-release sorts before release,
# and append debian revision "-1" (first packaging of this upstream version).
# E.g., "0.9.1-20260214.5f1d7ca" -> "0.9.1~20260214.5f1d7ca-1"
deb_version="${PKG_VERSION/-/\~}-1"
pkg_basename="clp-core_${deb_version}_${PKG_ARCH}.deb"

# --- Bundle binaries and libraries ------------------------------------------

DESTDIR="${staging}" \
PREFIX=/usr \
BIN_DIR="${BIN_DIR}" \
    "${script_dir}/../common/bundle-libs.sh"

# --- Create DEBIAN/control ----------------------------------------------------

mkdir -p "${staging}/DEBIAN"
installed_size_kib="$(du -sk "${staging}/usr" | awk '{print $1}')"
cat > "${staging}/DEBIAN/control" <<CTRL
Package: clp-core
Source: clp
Version: ${deb_version}
Architecture: ${PKG_ARCH}
Section: utils
Priority: optional
Installed-Size: ${installed_size_kib}
Maintainer: YScope Inc. <support@yscope.com>
Homepage: https://github.com/y-scope/clp
Depends: libc6 (>= 2.28), libstdc++6
Description: CLP core universal binaries for log compression and search
 Portable binaries built on manylinux_2_28 (glibc >= 2.28). Compatible with
 Debian 10+, Ubuntu 20.04+, and other glibc-based Debian derivatives.
 .
 Includes clp-s, clp, clo, clg, indexer, log-converter, and reducer-server.
CTRL

# --- Generate SBOM sidecar --------------------------------------------------
#
# The sidecar name appends `.sbom.cdx.json` to the full .deb filename so
# the package format is encoded in the sidecar's own filename and the
# package-to-sidecar relationship is a fixed-string suffix:
#   clp-core_<version>_<arch>.deb
#     -> clp-core_<version>_<arch>.deb.sbom.cdx.json
# See components/core/tools/packaging/SBOM.md for the merge model.

PKG_BASENAME="${pkg_basename}" \
PKG_NAME="clp-core" \
PKG_VERSION="${deb_version}" \
PKG_ARCH="${PKG_ARCH}" \
PKG_FORMAT="deb" \
STAGING_DIR="${staging}" \
STAGING_PREFIX="/usr" \
BUILD_DIR="${BIN_DIR}" \
DEPS_FAMILY="manylinux_2_28" \
OUTPUT_DIR="${output_dir}" \
    "${script_dir}/../common/generate-sbom.sh"

# This manifest drives SBOM scope overrides but is not part of the runtime
# package payload.
rm -f "${staging}/.bundled-os-packages.txt"

# --- Build .deb ---------------------------------------------------------------

echo "==> Building deb package..."
dpkg-deb --build "${staging}" "${output_dir}/"

pkg_path="${output_dir}/${pkg_basename}"
sbom_path="${pkg_path}.sbom.cdx.json"
if [[ ! -f "${pkg_path}" ]]; then
    echo "ERROR: expected package not found after dpkg-deb build: ${pkg_path}" >&2
    exit 1
fi

python3 "${script_dir}/../common/verify-package-sbom.py" \
    --package "${pkg_path}" \
    --sbom "${sbom_path}" \
    --update-package-metadata
