#!/usr/bin/env bash

remove_temp_file_and_prev_image() {
    rm -f "$temp_iid_file"

    if [[ -z "$new_image_id" ]]; then
        rm -f "$iid_file"
    elif [[ "$prev_image_id" == "$new_image_id" ]]; then
        return
    fi

    [[ -z "$prev_image_id" ]] && return

    docker image inspect "$prev_image_id" >/dev/null 2>&1 || return

    echo "Removing previous image $prev_image_id."
    docker image remove "$prev_image_id" || true
}

build_clp_image() {
    local image_name="$1"
    local repo_root="$2"
    local dockerfile="$3"
    shift 3

    iid_file="${repo_root}/build/${image_name}-image.id"

    prev_image_id=""
    if [[ -f "$iid_file" ]]; then
        prev_image_id=$(<"$iid_file")
    fi

    temp_iid_file="$(mktemp)"
    new_image_id=""
    trap remove_temp_file_and_prev_image EXIT

    build_cmd=(
        docker build
        --pull
        --iidfile "$temp_iid_file"
        "$repo_root"
        --file "$dockerfile"
        "$@"
    )

    if command -v git >/dev/null && git -C "$repo_root" rev-parse --is-inside-work-tree >/dev/null;
    then
        build_cmd+=(
            --label "org.opencontainers.image.revision=$(git -C "$repo_root" rev-parse HEAD)"
            --label "org.opencontainers.image.source=$(git -C "$repo_root" remote get-url origin)"
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
        docker tag "$new_image_id" "${image_name}:dev-${user}-${short_id}"
    fi
}
