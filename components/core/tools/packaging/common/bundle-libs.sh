#!/usr/bin/env bash

# Bundles CLP core binaries and their shared library dependencies into a
# staging directory suitable for packaging.
#
# This script is the shared core of all CLP package formats (deb, rpm, apk).
# It discovers shared library dependencies via ldd, copies non-system libraries
# into a lib directory, rewrites RPATHs with patchelf, and strips binaries.
#
# After this script completes, the staging directory contains:
#   <staging>/usr/bin/{clg,clo,clp,clp-s,indexer,log-converter,reducer-server}
#   <staging>/usr/lib/clp/{bundled .so files}
#
# Required environment variables:
#   STAGING_DIR  Path to the staging directory (will be created/cleaned)
#   BIN_DIR      Path to the directory containing compiled binaries
#
# Optional environment variables:
#   LIB_INSTALL_DIR  Library install path inside the package
#                    (default: /usr/lib/clp)

set -o errexit
set -o nounset
set -o pipefail

# --- Validate inputs --------------------------------------------------------

if [[ -z "${STAGING_DIR:-}" ]]; then
    echo "ERROR: STAGING_DIR is required" >&2
    exit 1
fi

if [[ -z "${BIN_DIR:-}" ]]; then
    echo "ERROR: BIN_DIR is required" >&2
    exit 1
fi

if [[ ! -d "${BIN_DIR}" ]]; then
    echo "ERROR: Binary directory not found: ${BIN_DIR}" >&2
    exit 1
fi

lib_install_dir="${LIB_INSTALL_DIR:-/usr/lib/clp}"

# --- Constants ---------------------------------------------------------------

# Keep in sync with CMakeLists.txt build targets and the %files list in
# universal-rpm/package.sh.
BINARIES=(clg clo clp clp-s indexer log-converter reducer-server)

# Libraries provided by the base system (libc, libstdc++, libgcc).
# These must NOT be bundled — the target system's versions are used instead.
# Covers both glibc (ld-linux, libc.so) and musl (ld-musl, libc.musl-*).
EXCLUDE_PATTERN="linux-vdso|ld-linux|ld-musl|libc\.so|libc\.musl|libm\.so|libpthread|libdl|librt\.so|libresolv|libstdc\+\+|libgcc_s"

# --- Prepare staging directory -----------------------------------------------

rm -rf "${STAGING_DIR}"
mkdir -p "${STAGING_DIR}/usr/bin" "${STAGING_DIR}${lib_install_dir}"

# --- Collect shared library dependencies -------------------------------------

echo "==> Collecting shared library dependencies..."
for bin in "${BINARIES[@]}"; do
    bin_path="${BIN_DIR}/${bin}"
    if [[ ! -f "${bin_path}" ]]; then
        echo "ERROR: ${bin} not found at ${bin_path}" >&2
        exit 1
    fi

    ldd_output=$(ldd "${bin_path}" 2>&1) || {
        echo "ERROR: ldd failed for ${bin}" >&2
        echo "${ldd_output}" >&2
        exit 1
    }
    echo "${ldd_output}" | while read -r line; do
        # Extract library path from ldd output (works on both glibc and musl).
        # glibc format: "libfoo.so.1 => /usr/lib/libfoo.so.1 (0x...)"
        # musl format:  "libfoo.so => /usr/lib/libfoo.so (0x...)"
        lib_path=$(echo "${line}" | awk '/=>/ {print $3}')
        [[ -n "${lib_path}" && "${lib_path}" == /* ]] || continue

        lib_name=$(basename "${lib_path}")
        echo "${lib_name}" | grep --quiet --extended-regexp "${EXCLUDE_PATTERN}" && continue

        if [[ ! -f "${STAGING_DIR}${lib_install_dir}/${lib_name}" ]]; then
            cp --dereference "${lib_path}" "${STAGING_DIR}${lib_install_dir}/${lib_name}"
            echo "    Bundled: ${lib_name}"
        fi
    done
done

# --- Patch bundled libraries' RPATH ------------------------------------------

echo "==> Patching bundled libraries..."
for lib in "${STAGING_DIR}${lib_install_dir}/"*.so*; do
    [[ -f "${lib}" ]] || continue
    patchelf --set-rpath "${lib_install_dir}" "${lib}"
done

# --- Install and patch binaries ----------------------------------------------

echo "==> Installing binaries..."
for bin in "${BINARIES[@]}"; do
    bin_path="${BIN_DIR}/${bin}"
    [[ -f "${bin_path}" ]] || continue

    cp "${bin_path}" "${STAGING_DIR}/usr/bin/${bin}"
    patchelf --set-rpath "${lib_install_dir}" "${STAGING_DIR}/usr/bin/${bin}"
    strip "${STAGING_DIR}/usr/bin/${bin}"
    echo "    Installed: ${bin}"
done

echo "==> Bundling complete."
