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

# shellcheck source=../common/core-binaries.sh
. "${script_dir}/../common/core-binaries.sh"
# shellcheck source=../common/package-metadata.sh
. "${script_dir}/../common/package-metadata.sh"
# shellcheck source=../common/package-inputs.sh
. "${script_dir}/../common/package-inputs.sh"
# shellcheck source=../common/package-output.sh
. "${script_dir}/../common/package-output.sh"

# --- Validate inputs ----------------------------------------------------------

clp_packaging_validate_package_inputs "universal-rpm"
output_dir="$(clp_packaging_resolve_output_dir)"
staging="/tmp/clp-rpm-staging"

# RPM version: replace hyphen with tilde so pre-release sorts before release.
# E.g., "0.9.1-20260214.5f1d7ca" -> "0.9.1~20260214.5f1d7ca"
rpm_version="${PKG_VERSION/-/\~}"

# --- Bundle binaries and libraries --------------------------------------------

STAGING_DIR="${staging}" \
BIN_DIR="${BIN_DIR}" \
    "${script_dir}/../common/bundle-libs.sh"

# --- Build .rpm via rpmbuild --------------------------------------------------

rpmbuild_dir="/tmp/clp-rpmbuild"
spec_path="${rpmbuild_dir}/SPECS/${CLP_CORE_PACKAGE_NAME}.spec"
rm -rf "${rpmbuild_dir}"
mkdir -p "${rpmbuild_dir}"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# Create the spec file
{
cat <<SPEC
Name:           ${CLP_CORE_PACKAGE_NAME}
Version:        %{pkg_version}
Release:        1
Summary:        ${CLP_CORE_PACKAGE_SUMMARY}
License:        ${CLP_CORE_PACKAGE_LICENSE}
URL:            ${CLP_CORE_PACKAGE_HOMEPAGE}
Packager:       ${CLP_CORE_PACKAGE_MAINTAINER}

AutoReqProv:    no
Requires:       glibc >= 2.28
Requires:       libstdc++

%description
Portable binaries built on manylinux_2_28 (glibc >= 2.28). Compatible with
RHEL 8+, Fedora 29+, AlmaLinux 8+, Rocky Linux 8+, and other glibc-based
RPM distributions.

${CLP_CORE_PACKAGE_INCLUDED_BINARIES}

%install
cp -a %{staging_dir}/* %{buildroot}/

%files
SPEC
for bin in "${CLP_CORE_BINARIES[@]}"; do
    printf '/usr/bin/%s\n' "${bin}"
done
cat <<'SPEC'
/usr/lib/clp/
SPEC
} > "${spec_path}"

echo "==> Building rpm package..."
rpmbuild \
    --define "_topdir ${rpmbuild_dir}" \
    --define "pkg_version ${rpm_version}" \
    --define "staging_dir ${staging}" \
    --target "${PKG_ARCH}" \
    --bb "${spec_path}"

# Copy the built RPM to the output directory
find "${rpmbuild_dir}/RPMS" -name "*.rpm" -exec cp {} "${output_dir}/" \;
