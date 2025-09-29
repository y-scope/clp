#!/usr/bin/env bash

set -eu
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
repo_root=${script_dir}/../../../
iid_file="${repo_root}/build/clp-package-image.id"

prev_image_id=""
if [[ -f "$iid_file" ]]; then
    prev_image_id=$(<"$iid_file")
fi

temp_iid_file="$(mktemp)"
new_image_id=""

remove_prev_image() {
    if [[ -n "$prev_image_id" && "$prev_image_id" != "$new_image_id" ]]; then
        if docker image inspect "$prev_image_id" >/dev/null 2>&1; then
            echo "Removing previous image $prev_image_id."
            docker image remove "$prev_image_id"
        fi
    fi
    rm -f "$temp_iid_file"
}
trap remove_prev_image EXIT

build_cmd=(
    docker build
    --iidfile "$temp_iid_file"
    "$repo_root"
    --file "${script_dir}/Dockerfile"
)

if command -v git >/dev/null && git -C "$script_dir" rev-parse --is-inside-work-tree >/dev/null ;
then
    build_cmd+=(
        --label "org.opencontainers.image.revision=$(git -C "$script_dir" rev-parse HEAD)"
        --label "org.opencontainers.image.source=$(git -C "$script_dir" remote get-url origin)"
    )
fi

"${build_cmd[@]}"

if [[ -s "$temp_iid_file" ]]; then
    new_image_id=$(<"$temp_iid_file")
    echo "$new_image_id" > "$iid_file"

    short_id=${new_image_id#sha256:}
    short_id=${short_id:0:4}
    docker tag "$new_image_id" "clp-package:dev-${USER}-${short_id}"
fi
