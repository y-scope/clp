#!/usr/bin/env bash

# Bundles CLP core binaries and their dylib dependencies into a staging
# directory suitable for tarballing. This is the macOS analogue of
# common/bundle-libs.sh.
#
# Key differences from the Linux version:
#   - otool -L is non-transitive (Linux ldd is transitive), so we walk the
#     dylib closure ourselves with a BFS over each binary's deps.
#   - install_name_tool rewrites both install names and dep paths to
#     @rpath-relative form; patchelf only rewrites RPATH on Linux.
#   - install_name_tool mutations invalidate the Mach-O code signature. Modern
#     macOS can refuse to load unsigned binaries after those rewrites, so we
#     re-apply an ad-hoc/pseudo signature after all mutations on each file.
#   - strip -S -x is macOS-native syntax (strip on Linux takes no flags).
#
# Compatible with Bash 3.2 (the version Apple ships at /bin/bash) so native
# macOS builds do not require `brew install bash`. That means no associative
# arrays; set membership uses files under a mktemp state dir.
#
# Uses DESTDIR/PREFIX convention matching bundle-libs.sh:
#   DESTDIR  Staging root — owned by this script, wiped on each run.
#   PREFIX   Runtime install prefix (e.g., "/usr/local" or "" for tarballs).
#
# After this script completes, the staging directory contains:
#   ${DESTDIR}${PREFIX}/bin/{clg,clo,clp,clp-s,indexer,log-converter,reducer-server}
#   ${DESTDIR}${PREFIX}/lib/clp/{bundled .dylib files}
#
# Required environment variables:
#   DESTDIR   Staging directory — wiped and recreated
#   BIN_DIR   Path to the directory containing compiled binaries
#
# Optional environment variables:
#   PREFIX    Runtime install prefix (default: "" for tarball-style layout)
#   MACHO_DYLIB_SEARCH_ROOTS
#             Colon-separated extra directories to search when a load command
#             uses a bare dylib name or a target-prefix install name.

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

# shellcheck source=../common/core-binaries.sh
. "${script_dir}/../common/core-binaries.sh"
# shellcheck source=../common/bundle-inputs.sh
. "${script_dir}/../common/bundle-inputs.sh"

# --- Validate inputs --------------------------------------------------------

# PREFIX unset → "" (tarball-style layout); PREFIX="/usr/local" → typical Unix
# install. Non-empty PREFIX must be an absolute path.
clp_packaging_validate_bundle_inputs
prefix="$(clp_packaging_resolve_bundle_prefix "")"
lib_install_dir="${prefix}/lib/clp"

# Native macOS uses the standard Xcode tool names. Cross builds can override
# them with OSXCross-prefixed tools.
otool_tool="${MACHO_OTOOL:-otool}"
install_name_tool_tool="${MACHO_INSTALL_NAME_TOOL:-install_name_tool}"
codesign_tool="${MACHO_CODESIGN:-codesign}"
allow_unsigned="${MACHO_ALLOW_UNSIGNED:-false}"
strip_tool="${MACHO_STRIP:-strip}"

for tool in "${otool_tool}" "${install_name_tool_tool}" "${codesign_tool}" "${strip_tool}"; do
    [[ "${tool}" == "skip" ]] && continue
    if ! command -v "${tool}" &>/dev/null; then
        echo "ERROR: Mach-O tool not found: ${tool}" >&2
        exit 1
    fi
done

if [[ "${codesign_tool}" == "skip" ]]; then
    if [[ "${allow_unsigned}" != "true" ]]; then
        echo "ERROR: MACHO_CODESIGN=skip requires MACHO_ALLOW_UNSIGNED=true." >&2
        echo "       Unsigned macOS binaries may fail to launch after install-name rewrites." >&2
        exit 1
    fi
    echo "WARNING: skipping Mach-O codesigning; this artifact is for local testing only." >&2
fi

# --- Constants ---------------------------------------------------------------

# Dylibs provided by macOS itself — locked to OS version, must NOT be bundled.
# Mirrors the role of EXCLUDE_PATTERN in bundle-libs.sh.
_is_system_dylib() {
    case "$1" in
        /usr/lib/*|/System/*) return 0 ;;
        *)                    return 1 ;;
    esac
}

# Strips parenthetical metadata from `otool -L` output and yields one dep path
# per line, skipping the header line (the file path printed first).
_list_deps() {
    "${otool_tool}" -L "$1" | tail -n +2 | awk '{print $1}'
}

# Removes every existing LC_RPATH entry from a Mach-O file. Homebrew dylibs
# and binaries often carry rpath entries pointing at /opt/homebrew/lib; if we
# leave those in the staged file, the dynamic linker on a target system that
# happens to have Homebrew installed could resolve our @rpath/* references
# against the host's dylibs instead of our bundled copy.
_strip_rpaths() {
    local file="$1"
    local rp
    while read -r rp; do
        [[ -n "${rp}" ]] || continue
        "${install_name_tool_tool}" -delete_rpath "${rp}" "${file}" 2>/dev/null || true
    done < <("${otool_tool}" -l "${file}" | awk '
        /^[[:space:]]+cmd LC_RPATH$/ { in_rpath = 1; next }
        in_rpath && /^[[:space:]]+path / { print $2; in_rpath = 0 }
    ')
}

# Re-applies an ad-hoc/pseudo signature. Native codesign preserves the original
# identifier; ldid is used by the Linux cross-build path when codesign is not
# available.
_resign() {
    local codesign_tool_name

    [[ "${codesign_tool}" == "skip" ]] && return 0
    codesign_tool_name="$(basename "${codesign_tool}")"
    if [[ "${codesign_tool_name}" == ldid* ]]; then
        "${codesign_tool}" -S "$1"
        return 0
    fi

    "${codesign_tool}" --force --sign - \
        --preserve-metadata=identifier,entitlements \
        "$1"
}

# --- Set-membership helpers (Bash-3.2 compatible) ----------------------------
#
# Replace `declare -A` (Bash 4+) with file-based sets:
#   visited_file    one absolute path per line
#   discovered_file one line per discovered dylib: "<basename>\t<original_path>"

state_dir=$(mktemp -d -t clp-bundle.XXXXXX)
trap 'rm -rf "${state_dir}"' EXIT
visited_file="${state_dir}/visited.txt"
discovered_file="${state_dir}/discovered.tsv"
unresolved_file="${state_dir}/unresolved.txt"
: > "${visited_file}"
: > "${discovered_file}"
: > "${unresolved_file}"

# True iff $1 has already been processed in the discovery walk.
_visited() {
    grep -qxF "$1" "${visited_file}"
}
_mark_visited() {
    printf '%s\n' "$1" >> "${visited_file}"
}

# True iff a dylib with basename $1 is already in the discovered set.
_discovered_by_basename() {
    grep -q "^$(printf '%s\n' "$1" | sed 's/[][\/.^$*]/\\&/g')	" "${discovered_file}"
}
_record_discovered() {
    printf '%s\t%s\n' "$1" "$2" >> "${discovered_file}"
}

_record_unresolved() {
    printf '%s\t%s\n' "$1" "$2" >> "${unresolved_file}"
}

# --- @rpath resolution helpers -----------------------------------------------
#
# CLP's deps (Boost, fmt, spdlog, ystdlib, log_surgeon, ...) are built into
# build/deps/cpp/<dep>-install/lib/ by `task deps:core`. CMake bakes those
# directories into the seed binaries as LC_RPATH entries; the deps themselves
# use install names like "@rpath/libboost_filesystem.dylib" and reference each
# other the same way. Crucially, sibling references inside those dylibs (e.g.
# libboost_filesystem.dylib → @rpath/libboost_system.dylib) often have NO
# LC_RPATH of their own — dyld resolves them via the *caller's* rpath at load
# time. So we collect LC_RPATH entries from all 7 seed binaries upfront and
# use that union to resolve every @rpath/* reference during the BFS, not just
# the ones directly attached to the file being scanned.
#
# (Skipping @rpath/* — as an earlier draft of this script did — silently
# drops everything CLP builds itself from the closure. The binary ends up
# rpath-rewritten correctly but the dylibs aren't actually bundled, and the
# tarball fails with "Library not loaded: @rpath/libboost_*.dylib" on first
# launch.)

SEED_RPATHS=()
DYLIB_SEARCH_ROOTS=()

_add_unique_dylib_search_root() {
    local root="$1"
    local existing

    [[ -n "${root}" && -d "${root}" ]] || return 0
    for existing in ${DYLIB_SEARCH_ROOTS[@]+"${DYLIB_SEARCH_ROOTS[@]}"}; do
        [[ "${existing}" == "${root}" ]] && return 0
    done
    DYLIB_SEARCH_ROOTS+=("${root}")
}

_collect_seed_rpaths() {
    local bin f rp seen existing
    for bin in "${CLP_CORE_BINARIES[@]}"; do
        f="${BIN_DIR}/${bin}"
        while read -r rp; do
            [[ -n "${rp}" ]] || continue
            seen=false
            for existing in ${SEED_RPATHS[@]+"${SEED_RPATHS[@]}"}; do
                [[ "${existing}" == "${rp}" ]] && { seen=true; break; }
            done
            ${seen} || SEED_RPATHS+=("${rp}")
        done < <("${otool_tool}" -l "${f}" | awk '
            /^[[:space:]]+cmd LC_RPATH$/ { in_rpath = 1; next }
            in_rpath && /^[[:space:]]+path / { print $2; in_rpath = 0 }
        ')
    done
}

_collect_dylib_search_roots() {
    local bin f rp expanded deps_cpp_dir install_dir macports_root extra_root
    local -a extra_roots

    IFS=':' read -r -a extra_roots <<< "${MACHO_DYLIB_SEARCH_ROOTS:-}"
    for extra_root in ${extra_roots[@]+"${extra_roots[@]}"}; do
        _add_unique_dylib_search_root "${extra_root}"
    done

    for bin in "${CLP_CORE_BINARIES[@]}"; do
        f="${BIN_DIR}/${bin}"
        for rp in ${SEED_RPATHS[@]+"${SEED_RPATHS[@]}"}; do
            expanded="${rp//@loader_path/$(dirname "${f}")}"
            expanded="${expanded//@executable_path/$(dirname "${f}")}"
            _add_unique_dylib_search_root "${expanded}"
        done
    done

    # Cross builds install source-built deps under build/deps/cpp/<dep>-install.
    # Those dylibs may appear in load commands as bare names, so the BFS needs
    # an explicit filename search path in addition to LC_RPATH resolution.
    deps_cpp_dir="$(cd "${BIN_DIR}/../deps/cpp" 2>/dev/null && pwd || true)"
    if [[ -n "${deps_cpp_dir}" ]]; then
        for install_dir in "${deps_cpp_dir}"/*-install; do
            [[ -d "${install_dir}" ]] || continue
            _add_unique_dylib_search_root "${install_dir}/lib"
            _add_unique_dylib_search_root "${install_dir}/lib/mariadb"
            _add_unique_dylib_search_root "${install_dir}/libexec/openssl3/lib"
            _add_unique_dylib_search_root "${install_dir}/libexec/openssl11/lib"
        done
    fi

    macports_root="${OSXCROSS_MACPORTS_ROOT:-}"
    if [[ -n "${macports_root}" ]]; then
        _add_unique_dylib_search_root "${macports_root}/lib"
        _add_unique_dylib_search_root "${macports_root}/lib/mariadb"
        _add_unique_dylib_search_root "${macports_root}/libexec/openssl3/lib"
        _add_unique_dylib_search_root "${macports_root}/libexec/openssl11/lib"
        _add_unique_dylib_search_root "${macports_root}/opt/openssl3/lib"
        _add_unique_dylib_search_root "${macports_root}/opt/openssl/lib"
    fi
}

# Resolves a Mach-O dep reference to an on-disk path. Prints the resolved
# path on stdout and returns 0; prints nothing and returns 1 if unresolvable
# (typically a system framework or a build-only ref).
#
# Supported reference forms:
#   /absolute/path           → returned as-is iff the file exists
#   /opt/local/<name>        → for OSXCross, mapped into OSXCROSS_MACPORTS_ROOT
#   @rpath/<name>            → tries each entry in SEED_RPATHS; first hit wins
#   @loader_path/<name>      → resolved relative to dirname of $1 (referencer)
#   @executable_path/<name>  → best-effort: also relative to dirname of $1
#   bare dylib name          → tries dirname of $1, then DYLIB_SEARCH_ROOTS
_resolve_dep_path() {
    local referencer="$1"
    local dep="$2"
    local ref_dir name rp resolved search_root
    case "${dep}" in
        @rpath/*)
            name="${dep#@rpath/}"
            ref_dir="$(dirname "${referencer}")"
            for rp in ${SEED_RPATHS[@]+"${SEED_RPATHS[@]}"}; do
                # @loader_path / @executable_path can appear inside the rpath
                # string itself (rare but legal Mach-O).
                rp="${rp//@loader_path/${ref_dir}}"
                rp="${rp//@executable_path/${ref_dir}}"
                if [[ -f "${rp}/${name}" ]]; then
                    printf '%s\n' "${rp}/${name}"
                    return 0
                fi
            done
            for search_root in ${DYLIB_SEARCH_ROOTS[@]+"${DYLIB_SEARCH_ROOTS[@]}"}; do
                if [[ -f "${search_root}/${name}" ]]; then
                    printf '%s\n' "${search_root}/${name}"
                    return 0
                fi
            done
            return 1 ;;
        @loader_path/*)
            name="${dep#@loader_path/}"
            resolved="$(dirname "${referencer}")/${name}"
            [[ -f "${resolved}" ]] && { printf '%s\n' "${resolved}"; return 0; }
            return 1 ;;
        @executable_path/*)
            name="${dep#@executable_path/}"
            resolved="$(dirname "${referencer}")/${name}"
            [[ -f "${resolved}" ]] && { printf '%s\n' "${resolved}"; return 0; }
            return 1 ;;
        /*)
            [[ -f "${dep}" ]] && { printf '%s\n' "${dep}"; return 0; }
            if [[ -n "${OSXCROSS_MACPORTS_ROOT:-}" && "${dep}" == /opt/local/* ]]; then
                resolved="${OSXCROSS_MACPORTS_ROOT}${dep#/opt/local}"
                [[ -f "${resolved}" ]] && { printf '%s\n' "${resolved}"; return 0; }
            fi
            return 1 ;;
        *)
            ref_dir="$(dirname "${referencer}")"
            [[ -f "${ref_dir}/${dep}" ]] && { printf '%s\n' "${ref_dir}/${dep}"; return 0; }
            for search_root in ${DYLIB_SEARCH_ROOTS[@]+"${DYLIB_SEARCH_ROOTS[@]}"}; do
                [[ -f "${search_root}/${dep}" ]] && {
                    printf '%s\n' "${search_root}/${dep}"
                    return 0
                }
            done
            return 1 ;;
    esac
}

_rewrite_bundled_load_commands() {
    local file="$1"
    local dep dep_base new_dep

    while read -r dep; do
        _is_system_dylib "${dep}" && continue

        dep_base=$(basename "${dep}")
        new_dep="@rpath/${dep_base}"
        [[ "${dep}" == "${new_dep}" ]] && continue

        if _discovered_by_basename "${dep_base}"; then
            "${install_name_tool_tool}" -change "${dep}" "${new_dep}" "${file}"
        fi
    done < <(_list_deps "${file}")
}

# --- Prepare staging directory -----------------------------------------------

rm -rf "${DESTDIR}"
mkdir -p "${DESTDIR}${prefix}/bin" "${DESTDIR}${lib_install_dir}"

# --- Phase 1: Discover the dylib closure -------------------------------------
#
# BFS over each binary's transitive dylib deps. `discovered` records the
# original absolute paths (as reported by otool) of every dylib we'll bundle;
# we use those exact strings later when calling install_name_tool -change to
# rewrite references.

worklist=()
for bin in "${CLP_CORE_BINARIES[@]}"; do
    bin_path="${BIN_DIR}/${bin}"
    if [[ ! -f "${bin_path}" ]]; then
        echo "ERROR: ${bin} not found at ${bin_path}" >&2
        exit 1
    fi
    worklist+=("${bin_path}")
done

# Collect the rpath search list once, from the seed binaries — used to
# resolve every @rpath/* dep encountered in the BFS below.
_collect_seed_rpaths
_collect_dylib_search_roots

echo "==> Discovering dylib closure..."
while [[ ${#worklist[@]} -gt 0 ]]; do
    item="${worklist[0]}"
    worklist=("${worklist[@]:1}")

    _visited "${item}" && continue
    _mark_visited "${item}"

    while read -r dep; do
        # When otool -L scans a dylib, the first dep line is the dylib's
        # own install name. Skip it so we don't recurse on ourselves.
        [[ "${dep}" == "${item}" ]] && continue

        # Resolve absolute paths, @rpath, @loader_path, @executable_path to
        # actual on-disk files. An unresolvable ref is almost always a
        # system framework (or, rarely, a build-only reference that won't
        # exist at runtime). Quietly skip system-style refs; warn on the
        # rest so a missing bundle is loud.
        if ! resolved=$(_resolve_dep_path "${item}" "${dep}"); then
            if ! _is_system_dylib "${dep}"; then
                _record_unresolved "${item}" "${dep}"
                echo "    WARNING: could not resolve '${dep}'" \
                    "(referenced from ${item}); skipping" >&2
            fi
            continue
        fi

        _is_system_dylib "${resolved}" && continue
        # Self-reference via resolved path (rare — happens when a dylib's
        # install name happens to equal its absolute on-disk path).
        [[ "${resolved}" == "${item}" ]] && continue

        base=$(basename "${resolved}")
        if ! _discovered_by_basename "${base}"; then
            _record_discovered "${base}" "${resolved}"
            worklist+=("${resolved}")
            echo "    Bundled: ${base}"
        fi
    done < <(_list_deps "${item}")
done

if [[ -s "${unresolved_file}" ]]; then
    echo "ERROR: Found unresolved non-system Mach-O dependencies:" >&2
    while IFS=$'\t' read -r referencer dep; do
        echo "  ${dep} (referenced from ${referencer})" >&2
    done < "${unresolved_file}"
    echo "Ensure each dependency is installed in the build prefix or add its directory" >&2
    echo "to MACHO_DYLIB_SEARCH_ROOTS before packaging." >&2
    exit 1
fi

# --- Phase 2: Copy + rewrite bundled dylibs ----------------------------------

discovered_count=$(wc -l < "${discovered_file}" | tr -d ' ')
echo "==> Copying ${discovered_count} dylibs into ${lib_install_dir}..."
while IFS=$'\t' read -r base src; do
    dest="${DESTDIR}${lib_install_dir}/${base}"
    # -L follows the symlink Homebrew often interposes between
    # /opt/homebrew/opt/<formula>/lib/ and the actual file in the Cellar.
    cp -L "${src}" "${dest}"
    chmod u+w "${dest}"
done < "${discovered_file}"

echo "==> Rewriting bundled dylibs..."
while IFS=$'\t' read -r base src; do
    dest="${DESTDIR}${lib_install_dir}/${base}"

    _strip_rpaths "${dest}"

    # Set the dylib's own install name. Without this, any binary linking
    # against it would record the original Homebrew path as the load command.
    "${install_name_tool_tool}" -id "@rpath/${base}" "${dest}"

    # Rewrite each reference to a bundled dep to use @rpath/<basename>.
    _rewrite_bundled_load_commands "${dest}"

    # Let this dylib find its siblings without depending on the binary's
    # rpath. Equivalent to bundle-libs.sh's `patchelf --set-rpath '$ORIGIN'`.
    "${install_name_tool_tool}" -add_rpath "@loader_path" "${dest}"

    _resign "${dest}"
done < "${discovered_file}"

# --- Phase 3: Install + rewrite binaries -------------------------------------

echo "==> Installing binaries..."
for bin in "${CLP_CORE_BINARIES[@]}"; do
    src="${BIN_DIR}/${bin}"
    dest="${DESTDIR}${prefix}/bin/${bin}"

    cp "${src}" "${dest}"
    chmod u+w "${dest}"

    _strip_rpaths "${dest}"

    _rewrite_bundled_load_commands "${dest}"

    # Binaries live in <prefix>/bin/; dylibs live in <prefix>/lib/clp/.
    # @loader_path is the binary's own dir, so ../lib/clp resolves to the
    # bundled-dylib dir. Equivalent to bundle-libs.sh's
    # `patchelf --set-rpath '$ORIGIN/../lib/clp'`.
    "${install_name_tool_tool}" -add_rpath "@loader_path/../lib/clp" "${dest}"

    "${strip_tool}" -S -x "${dest}"
    _resign "${dest}"

    echo "    Installed: ${bin}"
done

echo "==> Bundling complete."
