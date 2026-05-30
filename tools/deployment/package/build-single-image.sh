#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

remove_temp_file_and_base_tag() {
    if [[ -n "${temp_iid_file:-}" ]]; then
        rm -f "$temp_iid_file"
    fi
    if [[ -n "${base_image_ref:-}" ]]; then
        docker image remove "$base_image_ref" >/dev/null 2>&1 || true
    fi
}
trap remove_temp_file_and_base_tag EXIT

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
repo_root="${script_dir}/../../.."
base_iid_file="${repo_root}/build/clp-package-image.id"
single_iid_file="${repo_root}/build/clp-package-single-image.id"

if [[ ! -s "$base_iid_file" ]]; then
    echo >&2 \
        "Error: Base package image ID file '$base_iid_file' does not exist. Run 'task docker-images:package' first."
    exit 1
fi

base_image_id="$(tr -d '[:space:]' <"$base_iid_file")"
base_image_ref="clp-package:single-base"
docker tag "$base_image_id" "$base_image_ref"

temp_iid_file="$(mktemp)"
new_image_id=""

build_cmd=(
    docker build
    --network host
    --build-arg "CLP_PACKAGE_BASE_IMAGE=${base_image_ref}"
    --iidfile "$temp_iid_file"
    --file "${script_dir}/Dockerfile.single"
    "$script_dir"
)

if command -v git >/dev/null && git -C "$script_dir" rev-parse --is-inside-work-tree >/dev/null;
then
    build_cmd+=(
        --label "org.opencontainers.image.revision=$(git -C "$script_dir" rev-parse HEAD)"
    )
fi

"${build_cmd[@]}"

if [[ -s "$temp_iid_file" ]]; then
    new_image_id="$(<"$temp_iid_file")"
    echo "$new_image_id" > "$single_iid_file"

    docker tag "$new_image_id" "clp-package-single:latest"
fi
