#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

output_dir="${1:-/opt/clp/share/third-party-notices}"
debian_notices_dir="${output_dir}/debian-copyrights"

rm -rf "$output_dir"
mkdir -p "$debian_notices_dir"

cat > "${output_dir}/README.md" <<'EOF'
# CLP single-container image third-party notices

This directory is generated at image build time and records third-party
components installed in the CLP single-container image.

The image contains CLP plus third-party runtime services and libraries under
their own licenses. The image as a whole should not be described as Apache-2.0
only. CLP's Apache-2.0 license applies to CLP code; bundled third-party
components remain under their respective licenses.

MongoDB is bundled only as CLP's embedded results-cache implementation detail.
It is not intended to be exposed or marketed as a general-purpose MongoDB
service.

For public image publication, this directory is not sufficient by itself. The
release must also publish a matching SBOM and corresponding source bundle for
the exact image digest.
EOF

dpkg-query -W -f='${Package}\t${Version}\t${Architecture}\t${source:Package}\t${source:Version}\n' \
    | sort > "${output_dir}/debian-packages.tsv"

missing_copyrights="${output_dir}/missing-debian-copyrights.txt"
: > "$missing_copyrights"
while IFS=$'\t' read -r package _version _architecture _source_package _source_version; do
    copyright_file="/usr/share/doc/${package}/copyright"
    if [[ -f "$copyright_file" ]]; then
        cp "$copyright_file" "${debian_notices_dir}/${package}.copyright"
    else
        echo "$package" >> "$missing_copyrights"
    fi
done < "${output_dir}/debian-packages.tsv"

python3 <<'PY' > "${output_dir}/python-distributions.tsv"
from importlib import metadata

for dist in sorted(
    metadata.distributions(),
    key=lambda item: (item.metadata.get("Name") or item.name).lower(),
):
    name = dist.metadata.get("Name") or dist.name
    version = dist.version
    license_value = dist.metadata.get("License", "")
    home_page = dist.metadata.get("Home-page", "")
    print(f"{name}\t{version}\t{license_value}\t{home_page}")
PY

webui_package_lock="/opt/clp/var/www/webui/package-lock.json"
if [[ -f "$webui_package_lock" ]]; then
    cp "$webui_package_lock" "${output_dir}/webui-package-lock.json"
fi

find /opt/clp -maxdepth 4 \( -iname 'LICENSE*' -o -iname 'NOTICE*' -o -iname 'COPYING*' \) \
    -type f -print | sort > "${output_dir}/clp-license-files.txt"
