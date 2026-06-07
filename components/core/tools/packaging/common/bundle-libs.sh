#!/usr/bin/env bash

# Bundles CLP core binaries and their shared library dependencies into a
# staging directory suitable for packaging.
#
# This script is the shared core of all CLP package formats (deb, rpm, apk).
# It discovers shared library dependencies via ldd, copies non-system libraries
# into a lib directory, rewrites RPATHs with patchelf, and strips binaries.
#
# Uses DESTDIR/PREFIX convention:
#   DESTDIR  Staging/packaging root — owned by this script, wiped on each run
#            (e.g., /tmp/clp-deb-staging). Unlike GNU DESTDIR, this is required
#            and the directory is fully replaced.
#   PREFIX   Runtime install prefix  (e.g., /usr, /usr/local, or "")
#
# After this script completes, the staging directory contains:
#   ${DESTDIR}${PREFIX}/bin/{clg,clo,clp,clp-s,indexer,log-converter,reducer-server}
#   ${DESTDIR}${PREFIX}/lib/clp/{bundled .so files}
#
# Required environment variables:
#   DESTDIR  Staging directory — will be wiped and recreated
#   BIN_DIR  Path to the directory containing compiled binaries
#
# Optional environment variables:
#   PREFIX                 Runtime install prefix (default: /usr/local)
#   BUNDLE_COMPILER_LIBS   Bundle libstdc++ and libgcc_s (default: false)

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

# shellcheck source=core-binaries.sh
. "${script_dir}/core-binaries.sh"
# shellcheck source=bundle-inputs.sh
. "${script_dir}/bundle-inputs.sh"

# --- Validate inputs --------------------------------------------------------

# PREFIX unset → /usr/local; PREFIX="" → empty (for tarballs).
# Non-empty PREFIX must be an absolute path.
clp_packaging_validate_bundle_inputs
prefix="$(clp_packaging_resolve_bundle_prefix "/usr/local")"
lib_install_dir="${prefix}/lib/clp"

# --- Constants ---------------------------------------------------------------

# Libraries provided by the base system (libc, dynamic loader, POSIX libs).
# These must NOT be bundled — the target system's versions are used instead.
# Covers both glibc (ld-linux, libc.so) and musl (ld-musl, libc.musl-*).
SYSTEM_EXCLUDE_PATTERN="linux-vdso|ld-linux|ld-musl|libc\.so|libc\.musl|libm\.so|libmvec|libanl|libutil|libnsl|libpthread|libdl|librt\.so|libresolv"

# Package formats should use the target distro's compiler runtime packages.
# Tarballs do not have dependency metadata, so they can opt into bundling these.
COMPILER_RUNTIME_PATTERN="libstdc\+\+|libgcc_s"
if [[ "${BUNDLE_COMPILER_LIBS:-false}" == "true" ]]; then
    EXCLUDE_PATTERN="${SYSTEM_EXCLUDE_PATTERN}"
else
    EXCLUDE_PATTERN="${SYSTEM_EXCLUDE_PATTERN}|${COMPILER_RUNTIME_PATTERN}"
fi

# --- Prepare staging directory -----------------------------------------------

rm -rf "${DESTDIR}"
mkdir -p "${DESTDIR}${prefix}/bin" "${DESTDIR}${lib_install_dir}"

# --- Collect shared library dependencies -------------------------------------

echo "==> Collecting shared library dependencies..."
for bin in "${CLP_CORE_BINARIES[@]}"; do
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
        echo "${lib_name}" | grep -qE "${EXCLUDE_PATTERN}" && continue

        if [[ ! -f "${DESTDIR}${lib_install_dir}/${lib_name}" ]]; then
            cp --dereference "${lib_path}" "${DESTDIR}${lib_install_dir}/${lib_name}"
            echo "    Bundled: ${lib_name}"
        fi
    done
done

# --- Patch bundled libraries' RPATH ------------------------------------------

echo "==> Patching bundled libraries..."
for lib in "${DESTDIR}${lib_install_dir}/"*.so*; do
    [[ -f "${lib}" ]] || continue
    patchelf --set-rpath '$ORIGIN' "${lib}"
done

# --- Install and patch binaries ----------------------------------------------

echo "==> Installing binaries..."
for bin in "${CLP_CORE_BINARIES[@]}"; do
    bin_path="${BIN_DIR}/${bin}"
    [[ -f "${bin_path}" ]] || continue

    cp "${bin_path}" "${DESTDIR}${prefix}/bin/${bin}"
    patchelf --set-rpath '$ORIGIN/../lib/clp' "${DESTDIR}${prefix}/bin/${bin}"
    strip "${DESTDIR}${prefix}/bin/${bin}"
    echo "    Installed: ${bin}"
done

echo "==> Bundling complete."
