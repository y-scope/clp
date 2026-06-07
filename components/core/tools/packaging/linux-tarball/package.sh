#!/usr/bin/env bash

# Creates a universal Linux tarball from pre-built CLP core binaries.
#
# This script runs INSIDE a Linux package-builder container. It calls
# bundle-libs.sh to collect and patch binaries, then tars the staging tree.
#
# A glibc tarball is built on manylinux_2_28 (glibc >= 2.28). A musl tarball
# is built on musllinux_1_2 and is intended for musl-based distributions such
# as Alpine.
#
# Required environment variables:
#   PKG_VERSION  Package version (e.g., "0.9.1" or "0.9.1-20260214.5f1d7ca")
#   PKG_ARCH     Target package architecture: x86_64 or aarch64
#   PKG_LIBC     Target libc: glibc or musl
#   BIN_DIR      Path to the directory containing compiled binaries
#
# Optional environment variables:
#   OUTPUT_DIR   Where to write the .tar.gz file (default: current directory)

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

# shellcheck source=../common/package-inputs.sh
. "${script_dir}/../common/package-inputs.sh"
# shellcheck source=../common/package-metadata.sh
. "${script_dir}/../common/package-metadata.sh"
# shellcheck source=../common/package-output.sh
. "${script_dir}/../common/package-output.sh"

# --- Validate inputs --------------------------------------------------------

clp_packaging_validate_package_inputs "linux-tarball"
case "${PKG_ARCH}" in
    x86_64|aarch64) ;;
    *)
        echo "ERROR: linux-tarball only supports PKG_ARCH=x86_64 or aarch64, got: '${PKG_ARCH}'" >&2
        exit 1
        ;;
esac
case "${PKG_LIBC:-}" in
    glibc|musl) ;;
    *)
        echo "ERROR: linux-tarball only supports PKG_LIBC=glibc or musl, got: '${PKG_LIBC:-}'" >&2
        exit 1
        ;;
esac
output_dir="$(clp_packaging_resolve_output_dir)"

# Tarball convention follows the kubectl / node / go pattern: hyphens
# throughout, lowercase OS + arch suffix. The version string passed here has
# already been normalized by build.sh.
pkg_basename="${CLP_CORE_PACKAGE_NAME}-${PKG_VERSION}-linux-${PKG_LIBC}-${PKG_ARCH}"
staging_parent="${output_dir}/.staging"
staging="${staging_parent}/${pkg_basename}"

# --- Bundle binaries and shared libraries ----------------------------------

# PREFIX="" gives a tarball-style layout (top-level bin/, lib/clp/) rather
# than the Unix install layout (usr/bin/, usr/lib/clp/).
rm -rf "${staging_parent}"
mkdir -p "${staging_parent}"
DESTDIR="${staging}" \
PREFIX="" \
BIN_DIR="${BIN_DIR}" \
BUNDLE_COMPILER_LIBS=true \
    "${script_dir}/../common/bundle-libs.sh"

# --- Tar it ----------------------------------------------------------------

echo "==> Creating tarball..."
output_path="${output_dir}/${pkg_basename}.tar.gz"
rm -f "${output_path}"
tar -czf "${output_path}" -C "${staging_parent}" "${pkg_basename}"

# Clean up the staging tree. The tarball is self-contained.
rm -rf "${staging_parent}"

echo "==> Wrote ${output_path}"
ls -lh "${output_path}"
