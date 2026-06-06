#!/usr/bin/env bash

# Shared build-family activation for package builds.

clp_packaging_activate_build_family() {
    local repo_root="$1"
    local build_family="$2"
    local clean="${3:-false}"
    local dir_name target

    mkdir -p "${repo_root}/build-${build_family}" "${repo_root}/.task-${build_family}"

    # ln -sfn cannot replace a real directory. Refuse to replace user-owned
    # build state unless the caller has explicitly requested a clean build.
    for dir_name in build .task; do
        target="${repo_root}/${dir_name}"
        if [[ -L "${target}" || ! -e "${target}" ]]; then
            continue
        fi

        if [[ -d "${target}" ]]; then
            if [[ "${clean}" == "true" ]]; then
                echo "==> Replacing existing ${target} because --clean was supplied..."
                rm -rf "${target}"
                continue
            fi

            echo "ERROR: Refusing to replace existing real directory: ${target}" >&2
            echo "       Move or remove it, or rerun with --clean to replace it." >&2
            return 1
        fi

        echo "ERROR: Cannot replace existing non-directory path: ${target}" >&2
        echo "       Move or remove it before running the package build." >&2
        return 1
    done

    ln -sfn "build-${build_family}" "${repo_root}/build"
    ln -sfn ".task-${build_family}" "${repo_root}/.task"
}
