#!/usr/bin/env bash

# Shared high-level build steps for package builders.

clp_packaging_init_dependencies() {
    local repo_root="$1"

    echo "==> Initialising submodules..."
    "${repo_root}/tools/scripts/deps-download/init.sh"
}

clp_packaging_run_task() {
    local repo_root="$1"
    local cores="$2"
    local task_target="$3"

    (
        cd "${repo_root}" || exit 1
        CLP_CPP_MAX_PARALLELISM_PER_BUILD_TASK="${cores}" task "${task_target}"
    )
}

clp_packaging_build_cpp_dependencies() {
    local repo_root="$1"
    local cores="$2"

    echo "==> Building C++ dependencies..."
    clp_packaging_run_task "${repo_root}" "${cores}" deps:core
}

clp_packaging_build_core_with_task() {
    local repo_root="$1"
    local cores="$2"

    echo "==> Building core binaries..."
    clp_packaging_run_task "${repo_root}" "${cores}" core
}
