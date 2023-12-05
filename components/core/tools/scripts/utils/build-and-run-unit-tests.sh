#!/usr/bin/env bash

# Exit on any error
set -e
# Error on undefined variable
set -u

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

cmake -S "$src_dir" -B "$build_dir"
cmake --build "$build_dir"
cd "$build_dir"
if [ -z "${unit_tests_filter+x}" ]; then
    ./unitTest
else
    ./unitTest "$unit_tests_filter"
fi
