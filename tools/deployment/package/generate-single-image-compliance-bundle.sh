#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
repo_root="${script_dir}/../../.."

image_ref="${1:-clp-package-single:latest}"
output_dir="${2:-${repo_root}/build/clp-package-single-compliance}"

container_id=""
cleanup() {
    if [[ -n "$container_id" ]]; then
        docker rm "$container_id" >/dev/null 2>&1 || true
    fi
}
trap cleanup EXIT

rm -rf "$output_dir"
mkdir -p "$output_dir"

image_digest="$(docker image inspect "$image_ref" --format '{{index .RepoDigests 0}}' 2>/dev/null || true)"
image_id="$(docker image inspect "$image_ref" --format '{{.Id}}')"

cat > "${output_dir}/README.md" <<EOF
# CLP single-container image compliance bundle

Image reference: ${image_ref}
Image digest: ${image_digest:-not available locally}
Image ID: ${image_id}

This bundle records third-party components and notices for the exact local image
used to generate it. For public publication, upload this bundle together with a
matching corresponding-source bundle for copyleft and source-available
components.
EOF

container_id="$(docker create "$image_ref")"
docker cp "${container_id}:/opt/clp/share/third-party-notices/." \
    "${output_dir}/third-party-notices"
docker cp "${container_id}:/opt/clp/share/doc/clp-single-container/COMPLIANCE.md" \
    "${output_dir}/COMPLIANCE.md"

docker run --rm --entrypoint /bin/sh "$image_ref" -lc \
    "dpkg-query -W -f='\${Package}\t\${Version}\t\${Architecture}\t\${source:Package}\t\${source:Version}\n' | sort" \
    > "${output_dir}/debian-packages.tsv"

docker run --rm --entrypoint /bin/sh "$image_ref" -lc \
    "find /usr/share/doc -maxdepth 2 -name copyright -print | sort" \
    > "${output_dir}/debian-copyright-file-list.txt"

if command -v syft >/dev/null 2>&1; then
    syft "$image_ref" -o spdx-json > "${output_dir}/sbom.spdx.json"
    syft "$image_ref" -o cyclonedx-json > "${output_dir}/sbom.cyclonedx.json"
else
    echo >&2 "Error: syft is required to generate the release compliance bundle."
    exit 1
fi

cat > "${output_dir}/corresponding-source-notes.md" <<'EOF'
# Corresponding source notes

For public image publication, publish source artifacts matching the exact image
digest. At minimum, the source bundle should include:

* CLP source for the image revision.
* Source packages for GPL/LGPL/MPL and other source-required Debian packages.
* MongoDB Community Server source matching the bundled `mongodb-org-server`
  package version.
* Source and notices for Python and Node.js dependencies reported in the SBOM.

Do not rely on mutable upstream URLs as the only source offer for a released
image. Archive the exact source artifacts or publish a reviewed written source
offer that remains valid for the required period.
EOF

echo "Wrote compliance bundle to ${output_dir}"
