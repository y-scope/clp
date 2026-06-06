#!/usr/bin/env bash

# Shared package-version handling for CLP package builds.

clp_packaging_resolve_version() {
    local repo_root="$1"
    local version="${2-}"
    local git_date git_hash

    if [[ -z "${version}" ]]; then
        version=$(awk -F'"' '/G_PACKAGE_VERSION:/ { print $2; exit }' "${repo_root}/taskfile.yaml")
        if [[ -z "${version}" ]]; then
            echo "ERROR: Could not extract version from taskfile.yaml and --version not provided" >&2
            return 1
        fi
    fi

    # If the version has a pre-release suffix (anything after a hyphen, e.g.,
    # "-dev", "-beta", "-rc1"), replace it with a snapshot identifier for
    # reproducibility. Any existing suffix is replaced, so passing --version
    # "0.12.1-foo" still regenerates the snapshot from the current HEAD commit.
    # E.g., "0.12.1-dev" -> "0.12.1-20260603.abc1234".
    if [[ "${version}" == *-* ]]; then
        if ! git_date=$(git -C "${repo_root}" log -1 --format=%cd --date=format:%Y%m%d); then
            echo "ERROR: Could not resolve package snapshot date from git." >&2
            return 1
        fi
        if ! git_hash=$(git -C "${repo_root}" log -1 --format=%h); then
            echo "ERROR: Could not resolve package snapshot hash from git." >&2
            return 1
        fi
        version="${version%%-*}-${git_date}.${git_hash}"
    fi

    printf '%s\n' "${version}"
}
