#!/usr/bin/env bash

# Creates a universal .rpm package from pre-built CLP core binaries.
#
# This script runs INSIDE the rpm-builder container. It calls bundle-libs.sh
# to collect and patch binaries, then builds an .rpm with rpmbuild.
#
# The resulting package is "universal" — built on manylinux_2_28 (glibc >= 2.28),
# it's compatible with RHEL 8+, Fedora 29+, AlmaLinux 8+, Rocky Linux 8+, and
# other glibc-based RPM distributions.
#
# Required environment variables:
#   PKG_VERSION  Package version (e.g., "0.9.1" or "0.9.1-20260214.5f1d7ca")
#   PKG_ARCH     Target RPM architecture: x86_64 or aarch64
#   BIN_DIR      Path to the directory containing compiled binaries
#
# Optional environment variables:
#   OUTPUT_DIR   Where to write the .rpm file (default: current directory)

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

# --- Validate inputs ----------------------------------------------------------

if [[ -z "${PKG_VERSION:-}" ]]; then
    echo >&2 "ERROR: PKG_VERSION is required"
    exit 1
fi

if [[ -z "${PKG_ARCH:-}" ]]; then
    echo >&2 "ERROR: PKG_ARCH is required"
    exit 1
fi

if [[ -z "${BIN_DIR:-}" ]]; then
    echo >&2 "ERROR: BIN_DIR is required"
    exit 1
fi

output_dir="${OUTPUT_DIR:-$(pwd)}"
staging="/tmp/clp-rpm-staging"

# RPM version: replace hyphen with tilde so pre-release sorts before release.
# E.g., "0.9.1-20260214.5f1d7ca" -> "0.9.1~20260214.5f1d7ca"
rpm_version="${PKG_VERSION/-/\~}"
pkg_basename="clp-core-${rpm_version}-1.${PKG_ARCH}.rpm"

# --- Bundle binaries and libraries --------------------------------------------

DESTDIR="${staging}" \
PREFIX=/usr \
BIN_DIR="${BIN_DIR}" \
    "${script_dir}/../common/bundle-libs.sh"

# --- Generate SBOM sidecar ----------------------------------------------------
#
# The sidecar name appends `.sbom.cdx.json` to the full .rpm filename so
# the package format is encoded in the sidecar's own filename and the
# package-to-sidecar relationship is a fixed-string suffix:
#   clp-core-<rpm_version>-1.<arch>.rpm
#     -> clp-core-<rpm_version>-1.<arch>.rpm.sbom.cdx.json
# Release: 1 is hardcoded in the spec template that follows.
# See components/core/tools/packaging/SBOM.md for the merge model.

PKG_BASENAME="${pkg_basename}" \
PKG_NAME="clp-core" \
PKG_VERSION="${rpm_version}" \
PKG_ARCH="${PKG_ARCH}" \
PKG_FORMAT="rpm" \
STAGING_DIR="${staging}" \
STAGING_PREFIX="/usr" \
BUILD_DIR="${BIN_DIR}" \
DEPS_FAMILY="manylinux_2_28" \
OUTPUT_DIR="${output_dir}" \
    "${script_dir}/../common/generate-sbom.sh"

# --- Build .rpm via rpmbuild --------------------------------------------------

rpmbuild_dir="/tmp/clp-rpmbuild"
rm -rf "${rpmbuild_dir}"
mkdir -p "${rpmbuild_dir}"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# Create the spec file
cat > "${rpmbuild_dir}/SPECS/clp-core.spec" <<'SPEC'
Name:           clp-core
Version:        %{pkg_version}
Release:        1
Summary:        CLP core universal binaries for log compression and search
License:        Apache-2.0
URL:            https://github.com/y-scope/clp
Packager:       YScope Inc. <support@yscope.com>
Vendor:         YScope Inc.

AutoReqProv:    no
Requires:       glibc >= 2.28
Requires:       libstdc++

%description
Portable binaries built on manylinux_2_28 (glibc >= 2.28). Compatible with
RHEL 8+, Fedora 29+, AlmaLinux 8+, Rocky Linux 8+, and other glibc-based
RPM distributions.

Includes clp-s, clp, clo, clg, indexer, log-converter, and reducer-server.

%install
cp -a %{staging_dir}/* %{buildroot}/

%files
/usr/bin/clg
/usr/bin/clo
/usr/bin/clp
/usr/bin/clp-s
/usr/bin/indexer
/usr/bin/log-converter
/usr/bin/reducer-server
/usr/lib/clp/
SPEC

echo "==> Building rpm package..."
rpmbuild \
    --define "_topdir ${rpmbuild_dir}" \
    --define "__os_install_post %{nil}" \
    --define "pkg_version ${rpm_version}" \
    --define "staging_dir ${staging}" \
    --target "${PKG_ARCH}" \
    --bb "${rpmbuild_dir}/SPECS/clp-core.spec"

# Copy the built RPM to the output directory
built_rpm="$(find "${rpmbuild_dir}/RPMS" -name "${pkg_basename}" -print -quit)"
if [[ -z "${built_rpm}" ]]; then
    echo >&2 "ERROR: rpmbuild produced no matching package: ${pkg_basename}"
    exit 1
fi
cp "${built_rpm}" "${output_dir}/"

pkg_path="${output_dir}/${pkg_basename}"
sbom_path="${pkg_path}.sbom.cdx.json"
python3 "${script_dir}/../common/verify-package-sbom.py" \
    --package "${pkg_path}" \
    --sbom "${sbom_path}" \
    --format rpm \
    --update-package-metadata
