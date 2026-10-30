#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
lib_install_scripts_dir="${script_dir}/.."

# manylinux_2_28 enables portable builds but lacks support for system OpenSSL 3.x and static
# libraries. By manually installing both OpenSSL 1.x and 3.x to non-system directories, we gain
# the ability to switch versions during build configuration prior to compilation.
# Example: export CMAKE_PREFIX_PATH=/opt/openssl-<version>:$CMAKE_PREFIX_PATH
OPENSSL_VERSION_1X="${OPENSSL_VERSION_1X:-1.1.1w}"
OPENSSL_VERSION_3X="${OPENSSL_VERSION_3X:-3.5.9}"
"${lib_install_scripts_dir}/openssl.sh" "${OPENSSL_VERSION_1X}" /opt/openssl-1.x
"${lib_install_scripts_dir}/openssl.sh" "${OPENSSL_VERSION_3X}" /opt/openssl-3.x

# NOTE:
# 1. libarchive may statically link with LZMA, LZ4, and Zstandard, so we install them beforehand.
# 2. The versions of libarchive, LZMA, LZ4, and Zstandard available in manylinux_2_28's package
#    repositories are either dated or don't include static libraries, so we install more recent
#    versions from source.
"${lib_install_scripts_dir}/liblzma.sh" 5.8.1
"${lib_install_scripts_dir}/lz4.sh" 1.10.0
"${lib_install_scripts_dir}/zstandard.sh" 1.5.7
"${lib_install_scripts_dir}/libarchive.sh" 3.8.0
