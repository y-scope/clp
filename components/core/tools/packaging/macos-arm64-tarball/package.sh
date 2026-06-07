#!/usr/bin/env bash

# Packages pre-built CLP core binaries into a macOS tarball.
#
# This script runs either natively on macOS or inside the Dockerized OSXCross
# build environment. It calls bundle-dylibs.sh to populate a staging tree, then
# tars it.
#
# The resulting tarball is self-contained: extracting it on any macOS host for
# the target architecture and OS version family yields a working CLP install
# without needing Homebrew or any other dep manager. (Note: the first run will
# be blocked by Gatekeeper unless the user clears the quarantine attribute; see
# README.md.)
#
# Required environment variables:
#   PKG_VERSION  Package version (e.g., "0.12.1" or "0.12.1-20260603.abc1234")
#   PKG_ARCH     Target package architecture: arm64 or x86_64
#   BIN_DIR      Path to the directory containing compiled binaries
#
# Optional environment variables:
#   OUTPUT_DIR   Where to write the .tar.gz file (default: current directory)

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

# shellcheck source=defaults.sh
. "${script_dir}/defaults.sh"
# shellcheck source=../common/package-inputs.sh
. "${script_dir}/../common/package-inputs.sh"

# --- Validate inputs --------------------------------------------------------

clp_packaging_validate_package_inputs "macos-tarball"
pkg_arch="$(clp_macos_normalize_arch "${PKG_ARCH}")"
output_dir="$(clp_packaging_output_dir)"
mkdir -p "${output_dir}"

# Tarball convention follows the kubectl / node / go pattern: hyphens
# throughout, lowercase OS + arch suffix. The version string passed here has
# already been normalized by build-package.sh: release versions are kept as-is,
# while hyphenated pre-release suffixes are replaced with a git-date/hash
# snapshot suffix.
pkg_basename="clp-core-${PKG_VERSION}-macos-${pkg_arch}"
staging_parent="${output_dir}/.staging"
staging="${staging_parent}/${pkg_basename}"

# --- Bundle binaries and dylibs ---------------------------------------------

# PREFIX="" gives a tarball-style layout (top-level bin/, lib/clp/) rather
# than the Unix install layout (usr/local/bin/, usr/local/lib/clp/).
rm -rf "${staging_parent}"
mkdir -p "${staging_parent}"
DESTDIR="${staging}" \
PREFIX="" \
BIN_DIR="${BIN_DIR}" \
    "${script_dir}/bundle-dylibs.sh"

# --- Tar it -----------------------------------------------------------------

echo "==> Creating tarball..."
output_path="${output_dir}/${pkg_basename}.tar.gz"
rm -f "${output_path}"
tar -czf "${output_path}" -C "${staging_parent}" "${pkg_basename}"

# Clean up the staging tree — the tarball is self-contained, nothing else
# needs it.
rm -rf "${staging_parent}"

echo "==> Wrote ${output_path}"
ls -lh "${output_path}"
