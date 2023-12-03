#!/usr/bin/env bash

# Exit on any error
set -e
# Error on undefined variable
set -u

cUsage="Usage: ${BASH_SOURCE[0]} <source-dir> <build-dir>"
if [ "$#" -lt 1 ] ; then
    echo "$cUsage"
    exit 1
fi
src_dir="$1"
build_dir="$2"

cmake -S "$src_dir" -B "$build_dir"
cmake --build "$build_dir"
cd "$build_dir"
./unitTest
