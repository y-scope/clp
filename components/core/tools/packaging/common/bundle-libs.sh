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
#   ${DESTDIR}/.bundled-os-packages.txt  Names of OS packages that own the
#                                        bundled .so files (one per line,
#                                        sorted, deduped). Consumed by
#                                        generate-sbom.sh to force the
#                                        corresponding syft components to
#                                        scope: required even if the
#                                        build-only denylist would otherwise
#                                        mark them optional. This is the
#                                        authoritative source of truth for
#                                        "what ships at runtime".
#
# Required environment variables:
#   DESTDIR  Staging directory — will be wiped and recreated
#   BIN_DIR  Path to the directory containing compiled binaries
#
# Optional environment variables:
#   PREFIX  Runtime install prefix (default: /usr/local)

set -o errexit
set -o nounset
set -o pipefail

# --- Validate inputs --------------------------------------------------------

if [[ -z "${DESTDIR:-}" ]]; then
    echo "ERROR: DESTDIR is required" >&2
    exit 1
fi

if [[ "${DESTDIR}" != /* ]]; then
    echo "ERROR: DESTDIR must be an absolute path, got: '${DESTDIR}'" >&2
    exit 1
fi

if [[ "${DESTDIR}" == "/" ]]; then
    echo "ERROR: DESTDIR must not be /" >&2
    exit 1
fi

if [[ -z "${BIN_DIR:-}" ]]; then
    echo "ERROR: BIN_DIR is required" >&2
    exit 1
fi

if [[ ! -d "${BIN_DIR}" ]]; then
    echo "ERROR: Binary directory not found: '${BIN_DIR}'" >&2
    exit 1
fi

# PREFIX unset → /usr/local; PREFIX="" → empty (for tarballs).
# Non-empty PREFIX must be an absolute path.
prefix="${PREFIX-/usr/local}"
if [[ -n "${prefix}" && "${prefix}" != /* ]]; then
    echo "ERROR: PREFIX must start with '/' or be empty, got: '${prefix}'" >&2
    exit 1
fi
lib_install_dir="${prefix}/lib/clp"

# --- Constants ---------------------------------------------------------------

# Keep in sync with CMakeLists.txt build targets and the %files list in
# universal-rpm/package.sh.
BINARIES=(clg clo clp clp-s indexer log-converter reducer-server)

# Libraries provided by the base system (libc, libstdc++, libgcc).
# These must NOT be bundled — the target system's versions are used instead.
# Covers both glibc (ld-linux, libc.so) and musl (ld-musl, libc.musl-*).
EXCLUDE_PATTERN="linux-vdso|ld-linux|ld-musl|libc\.so|libc\.musl|libm\.so|libmvec|libanl|libutil|libnsl|libpthread|libdl|librt\.so|libresolv|libstdc\+\+|libgcc_s"

# --- Prepare staging directory -----------------------------------------------

rm -rf "${DESTDIR}"
mkdir -p "${DESTDIR}${prefix}/bin" "${DESTDIR}${lib_install_dir}"

# --- Collect shared library dependencies -------------------------------------

# Per-bin ldd output is filtered by EXCLUDE_PATTERN, then each surviving
# library path is recorded so the package-resolution step below can map
# them back to OS package names. The file is unsorted/duplicated here;
# uniq+sort happens once per staging dir below.
bundled_sources_tmp="${DESTDIR}/.bundled-lib-sources.tmp"
: > "${bundled_sources_tmp}"

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
        echo "${lib_name}" | grep -qE "${EXCLUDE_PATTERN}" && continue

        if [[ ! -f "${DESTDIR}${lib_install_dir}/${lib_name}" ]]; then
            cp --dereference "${lib_path}" "${DESTDIR}${lib_install_dir}/${lib_name}"
            echo "    Bundled: ${lib_name}"
        fi
        # Record the source path even on subsequent occurrences so the
        # package resolver sees the full set if two binaries pulled in
        # the same lib from different paths (rare but possible).
        echo "${lib_path}" >> "${bundled_sources_tmp}"
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
for bin in "${BINARIES[@]}"; do
    bin_path="${BIN_DIR}/${bin}"
    [[ -f "${bin_path}" ]] || continue

    cp "${bin_path}" "${DESTDIR}${prefix}/bin/${bin}"
    patchelf --set-rpath '$ORIGIN/../lib/clp' "${DESTDIR}${prefix}/bin/${bin}"
    strip "${DESTDIR}${prefix}/bin/${bin}"
    echo "    Installed: ${bin}"
done

# --- Resolve bundled libraries to owning OS packages -------------------------
#
# Map each bundled .so back to the OS package that owns it, then write the
# deduped name set to .bundled-os-packages.txt. generate-sbom.py uses this
# file to force scope: required on the corresponding syft components,
# overriding the build-only denylist. The structural override is what makes
# the denylist safe to evolve: if a future iteration accidentally lists a
# package whose .so is in /usr/lib/clp/, this file still keeps it `required`.
#
# Resolver per builder image:
#   manylinux (rpm db):  rpm -qf --queryformat '%{NAME}\n' <path>
#   musllinux (apk db):  apk info --who-owns <path>, strip "-<ver>-r<rel>"
#                        suffix via a regex on Alpine's version grammar
#                        (always ends in `-r<digits>`; the chunk before
#                        starts with a digit and is .-separated).
#   debian/ubuntu (dpkg, defensive — not the current builder set): dpkg -S
#
# If no recognized package manager is available, write an empty file and
# log a warning — the SBOM merger treats an empty manifest as "no
# structural override available" and falls back to denylist-only policy.

echo "==> Resolving bundled libraries to OS packages..."
bundled_pkgs_file="${DESTDIR}/.bundled-os-packages.txt"
: > "${bundled_pkgs_file}"
if [[ -s "${bundled_sources_tmp}" ]]; then
    sort -u "${bundled_sources_tmp}" -o "${bundled_sources_tmp}"
    n_libs=$(wc -l < "${bundled_sources_tmp}" | tr -d ' ')

    if command -v rpm &>/dev/null; then
        resolver_kind="rpm"
        while read -r p; do
            rpm -qf --queryformat '%{NAME}\n' "${p}" 2>/dev/null || true
        done < "${bundled_sources_tmp}" \
            | sort -u > "${bundled_pkgs_file}"
    elif command -v apk &>/dev/null; then
        resolver_kind="apk"
        while read -r p; do
            apk info --who-owns "${p}" 2>/dev/null || true
        done < "${bundled_sources_tmp}" \
            | python3 -c '
import sys, re
# Format: "<path> is owned by <pkgname>-<version>-r<release>"
# <version> always starts with a digit and contains no dashes; the trailing
# "-r<digits>" is mandatory. Strip both to recover <pkgname>.
pat = re.compile(r"is owned by (.+?)-\d[^-\s]*-r\d+\s*$")
for line in sys.stdin:
    m = pat.search(line)
    if m:
        print(m.group(1))
' \
            | sort -u > "${bundled_pkgs_file}"
    elif command -v dpkg &>/dev/null; then
        resolver_kind="dpkg"
        while read -r p; do
            dpkg -S "${p}" 2>/dev/null | awk -F: '{print $1; exit}'
        done < "${bundled_sources_tmp}" \
            | sort -u > "${bundled_pkgs_file}"
    else
        echo "WARNING: no rpm/apk/dpkg available; bundled-package manifest" \
             "will be empty (denylist override disabled)" >&2
        resolver_kind="none"
    fi

    n_pkgs=$(wc -l < "${bundled_pkgs_file}" | tr -d ' ')
    echo "    Resolved ${n_libs} bundled libraries → ${n_pkgs} OS packages" \
         "via ${resolver_kind}"
    # Warn (but do not fail) if the resolver mapped 0 packages — that
    # indicates either an empty staging dir or a resolver bug. The SBOM
    # build will still succeed with no structural override.
    if [[ "${n_pkgs}" == "0" && "${resolver_kind}" != "none" ]]; then
        echo "WARNING: ${resolver_kind} resolved 0 owning packages for" \
             "${n_libs} bundled libraries — check the resolver output" >&2
    fi
fi
rm -f "${bundled_sources_tmp}"

echo "==> Bundling complete."
