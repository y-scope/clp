#!/usr/bin/env bash

# Exit on any error
set -e
# Error on undefined variable
set -u

# Builds core.
#
# @param src_dir
# @param build_dir
# @param use_static_libs Whether to use static libraries when building.
# @param num_jobs Max number of jobs to run when building.
build () {
    src_dir="$1"
    build_dir="$2"
    use_static_libs="$3"
    num_jobs="$4"

    cmake_flags=()
    if [ "$use_static_libs" = true ]; then
        cmake_flags+=(-DCLP_USE_STATIC_LIBS=ON)
    else
        cmake_flags+=(-DCLP_USE_STATIC_LIBS=OFF)
    fi

    rm -rf "$build_dir"
    cmake "${cmake_flags[@]}" -S "$src_dir" -B "$build_dir"
    cmake --build "$build_dir" --parallel "$num_jobs"
}

cUsage="Usage: ${BASH_SOURCE[0]} <source-dir> <build-dir>[ <unit-tests-filter>]"
if [ "$#" -lt 2 ]; then
    echo "$cUsage"
    exit 1
fi
src_dir="$1"
build_dir="$2"
if [ "$#" -gt 2 ]; then
    unit_tests_filter="$3"
fi

num_cores="$(getconf _NPROCESSORS_ONLN)"

# NOTE: The static-linking build must be last since those binaries are added to the core binaries
# container images.
build "$src_dir" "$build_dir" false "$num_cores"
build "$src_dir" "$build_dir" true "$num_cores"

cd "$build_dir"
if [ -z "${unit_tests_filter+x}" ]; then
    ./unitTest
else
    ./unitTest "$unit_tests_filter"
fi
