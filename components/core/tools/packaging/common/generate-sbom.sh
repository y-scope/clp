#!/usr/bin/env bash

# Generate a CycloneDX 1.5 SBOM sidecar for a CLP core package.
#
# This wrapper runs inside the package-builder container, after
# bundle-libs.sh has finished and before the format-specific build tool
# (dpkg-deb, rpmbuild, or abuild) runs. It performs one syft scan
# restricted to OS-package catalogers and invokes generate-sbom.py to
# merge its output with the CLP build manifests.
#
# Output: ${OUTPUT_DIR}/${PKG_BASENAME}.sbom.cdx.json
#
# Failure model: any non-zero exit aborts the package build. The sidecar
# is required by the downstream Trivy gate; producing a package without a
# matching SBOM would silently bypass that gate. The syft pass is also
# required (not best-effort): syft is the sole source of OS-package
# coverage in the merged SBOM, and the merger enforces a minimum
# OS-component count to catch a silently-empty scan.
#
# Required environment variables:
#   PKG_BASENAME    Sidecar filename stem (e.g., clp-core_<ver>_<arch>.deb);
#                   the sidecar is written as ${PKG_BASENAME}.sbom.cdx.json
#   PKG_NAME        SBOM root component name (e.g., clp-core)
#   PKG_VERSION     SBOM root component version (format-specific string)
#   PKG_ARCH        Package architecture (deb: amd64/arm64,
#                   rpm/apk: x86_64/aarch64)
#   PKG_FORMAT      Package format: deb, rpm, or apk
#   STAGING_DIR     Staging directory populated by bundle-libs.sh
#   STAGING_PREFIX  Install prefix used inside the staging dir (e.g., /usr)
#   BUILD_DIR       CMake build directory (reserved for future use)
#   DEPS_FAMILY     manylinux_2_28 or musllinux_1_2
#   OUTPUT_DIR      Destination directory for the SBOM sidecar

set -o errexit
set -o nounset
set -o pipefail

# script_dir resolves to <repo>/components/core/tools/packaging/common
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
repo_root="$(cd "${script_dir}/../../../../.." && pwd)"

# --- Validate inputs ----------------------------------------------------------

required_vars=(
    PKG_BASENAME PKG_NAME PKG_VERSION PKG_ARCH PKG_FORMAT
    STAGING_DIR STAGING_PREFIX BUILD_DIR DEPS_FAMILY OUTPUT_DIR
)
for var in "${required_vars[@]}"; do
    if [[ -z "${!var:-}" ]]; then
        echo "ERROR: ${var} is required" >&2
        exit 1
    fi
done

if ! command -v syft &>/dev/null; then
    echo "ERROR: syft is not installed in the builder image" >&2
    exit 1
fi
if ! command -v python3 &>/dev/null; then
    echo "ERROR: python3 is not installed in the builder image" >&2
    exit 1
fi

deps_yaml="${repo_root}/taskfiles/deps/main.yaml"
source_install_script="${repo_root}/components/core/tools/scripts/lib_install/${DEPS_FAMILY}/install-packages-from-source.sh"
init_sh="${repo_root}/tools/scripts/deps-download/init.sh"
for path in "${deps_yaml}" "${source_install_script}" "${init_sh}"; do
    if [[ ! -f "${path}" ]]; then
        echo "ERROR: required manifest input missing: ${path}" >&2
        exit 1
    fi
done

# --- Run syft -----------------------------------------------------------------

# Restrict syft to the three OS package-database catalogers that match
# the formats this pipeline targets: rpm (manylinux_2_28), dpkg (deb
# builds reading the AlmaLinux 8 rpm db inside the manylinux container),
# and apk (musllinux_1_2). The broader `--override-default-catalogers os`
# tag also pulls in binary-classifier-cataloger and elf-binary-package-
# cataloger, which inventory every ELF in the builder image and would
# add tens of thousands of file-level entries with no PURL — bloat that
# is not part of CLP's CVE surface and which our own scan_bundled_libs()
# already covers for what actually ships in the package.
syft_out="/tmp/clp-syft.cdx.json"
rm -f "${syft_out}"

echo "==> Running syft for OS package inventory..."
if ! syft scan dir:/ \
    --override-default-catalogers "rpm-db-cataloger,dpkg-db-cataloger,apk-db-cataloger" \
    --output "cyclonedx-json@1.5=${syft_out}" \
    --quiet; then
    echo "ERROR: syft scan failed" >&2
    exit 1
fi
if [[ ! -s "${syft_out}" ]]; then
    echo "ERROR: syft produced no output at ${syft_out}" >&2
    exit 1
fi

# --- Merge into final sidecar -------------------------------------------------

sbom_out="${OUTPUT_DIR}/${PKG_BASENAME}.sbom.cdx.json"
mkdir -p "${OUTPUT_DIR}"

# bundle-libs.sh emits .bundled-os-packages.txt with the OS package names
# that own the bundled .so files in ${STAGING_DIR}${STAGING_PREFIX}/lib/clp/.
# The merger uses this as a structural override against the build-only
# denylist: any package in this file is forced to scope: required even if
# the denylist would otherwise tag it optional. Pass through if present;
# omit the flag if absent so the merger falls back to denylist-only policy.
bundled_pkgs_arg=()
bundled_pkgs_file="${STAGING_DIR}/.bundled-os-packages.txt"
if [[ -s "${bundled_pkgs_file}" ]]; then
    bundled_pkgs_arg=(--bundled-packages "${bundled_pkgs_file}")
fi

echo "==> Merging syft output with CLP manifests..."
python3 "${script_dir}/generate-sbom.py" \
    --syft-input "${syft_out}" \
    --deps-yaml "${deps_yaml}" \
    --source-install-script "${source_install_script}" \
    --init-sh "${init_sh}" \
    --staging-dir "${STAGING_DIR}" \
    --staging-prefix "${STAGING_PREFIX}" \
    --deps-family "${DEPS_FAMILY}" \
    --pkg-name "${PKG_NAME}" \
    --pkg-version "${PKG_VERSION}" \
    --pkg-arch "${PKG_ARCH}" \
    --pkg-format "${PKG_FORMAT}" \
    "${bundled_pkgs_arg[@]}" \
    --output "${sbom_out}"
