#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

remove_temp_file_and_prev_image() {
    rm -f "$temp_iid_file"

    [[ -z "$prev_image_id" || "$prev_image_id" == "$new_image_id" ]] && return

    docker image inspect "$prev_image_id" >/dev/null 2>&1 || return

    echo "Removing previous image $prev_image_id."
    docker image remove "$prev_image_id" || true
}
trap remove_temp_file_and_prev_image EXIT

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
repo_root="${script_dir}/../../../"
iid_file="${repo_root}/build/clp-package-image.id"

prev_image_id=""
if [[ -f "$iid_file" ]]; then
    prev_image_id=$(<"$iid_file")
fi

temp_iid_file="$(mktemp)"
new_image_id=""

build_cmd=(
    docker build
    --pull
    --iidfile "$temp_iid_file"
    "$repo_root"
    --file "${script_dir}/Dockerfile"
)

if command -v git >/dev/null && git -C "$script_dir" rev-parse --is-inside-work-tree >/dev/null;
then
    build_cmd+=(
        --label "org.opencontainers.image.revision=$(git -C "$script_dir" rev-parse HEAD)"
        --label "org.opencontainers.image.source=$(git -C "$script_dir" remote get-url origin)"
    )
fi

"${build_cmd[@]}"

if [[ -s "$temp_iid_file" ]]; then
    new_image_id="$(<"$temp_iid_file")"
    echo "$new_image_id" > "$iid_file"

    user="${USER:-$(whoami 2>/dev/null)}" \
        || user=$(id -un 2>/dev/null) \
        || user=$(id -u 2>/dev/null) \
        || user="unknown";
    short_id="${new_image_id#sha256:}"
    short_id="${short_id:0:4}"
    docker tag "$new_image_id" "clp-package:dev-${user}-${short_id}"
fi
