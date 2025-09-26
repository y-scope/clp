#!/usr/bin/env bash

set -eu
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
repo_root=${script_dir}/../../../
iid_file="${repo_root}/build/clp-package-image.id"

# Remove the previous image to save space, but only after the build so its contents can be reused.
prev_image_id=""
if [[ -f "$iid_file" ]]; then
    prev_image_id=$(cat "$iid_file")
fi
cleanup() {
    if [[ -n "$prev_image_id" ]] && docker image inspect "$prev_image_id" >/dev/null 2>&1; then
        echo "Removing previous image $prev_image_id"
        docker image remove "$prev_image_id"
    fi
}
trap cleanup EXIT

build_cmd=(
    docker build
    --iidfile "$iid_file"
    --tag "clp-package:dev-${USER}-$(date +%s)"
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
