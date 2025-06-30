#!/usr/bin/env bash

set -eu
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
lib_install_scripts_dir="${script_dir}/.."

# NOTE: The remaining installation scripts depend on boost, so we install it beforehand.
"${lib_install_scripts_dir}/install-boost.sh" 1.87.0

# NOTE:
# 1. libarchive may statically link with LZMA, LZ4, and Zstandard, so we install them beforehand.
# 2. The versions of libarchive, LZMA, LZ4, and Zstandard available in manylinux_2_28's package
#    repositories are either dated or don't include static libraries, so we install more recent
#    versions from source.
"${lib_install_scripts_dir}/liblzma.sh" 5.8.1
"${lib_install_scripts_dir}/lz4.sh" 1.10.0
"${lib_install_scripts_dir}/zstandard.sh" 1.5.7
"${lib_install_scripts_dir}/libarchive.sh" 3.8.0

"${lib_install_scripts_dir}/msgpack.sh" 7.0.0
