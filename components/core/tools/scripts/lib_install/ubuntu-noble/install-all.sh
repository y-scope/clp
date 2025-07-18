#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u
set -o pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

echo "Installing prebuilt packages…"
"${script_dir}/install-prebuilt-packages.sh"

echo "Installing packages from source…"
"${script_dir}/install-packages-from-source.sh"

# TODO: remove this workaround when CMake >= 3.22 is packaged (see issue #795)
"${script_dir}/../check-cmake-version.sh"
