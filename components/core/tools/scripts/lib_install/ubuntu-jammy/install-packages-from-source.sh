#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
scripts_prefix=$script_dir/..

"$scripts_prefix"/fmtlib.sh 8.0.1
"$scripts_prefix"/libarchive.sh 3.5.1
"$scripts_prefix"/lz4.sh 1.8.2
"$scripts_prefix"/mongoc.sh 1.24.4
"$scripts_prefix"/mongocxx.sh 3.8.0
"$scripts_prefix"/msgpack.sh 6.0.0
"$scripts_prefix"/spdlog.sh 1.9.2
"$scripts_prefix"/zstandard.sh 1.4.9
