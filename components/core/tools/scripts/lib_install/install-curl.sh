#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

cUsage="Usage: ${BASH_SOURCE[0]} <version>"
if [ "$#" -lt 1 ] ; then
    echo $cUsage
    exit
fi
version=$1

echo "Checking for elevated privileges..."
if [ ${EUID:-$(id -u)} -ne 0 ] ; then
  sudo echo "Script can elevate privileges."
fi

# Get number of cpu cores, capped by available memory (min 2 GB per core)
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
source "${script_dir}/../../../../../tools/scripts/compute-cpp-max-parallelism.sh"
num_cpus=$compute_cpp_max_parallelism_result

package_name=curl

# Create temp dir for installation
temp_dir=/tmp/${package_name}-installation
mkdir -p $temp_dir

cd $temp_dir

# Download source
tar_filename=curl-${version}.tar.gz
curl -fsSLO https://curl.se/download/${tar_filename}
tar xzf ${tar_filename}
cd curl-${version}

# Build
./configure --with-openssl
make -j${num_cpus}

# Install
if [ ${EUID:-$(id -u)} -ne 0 ] ; then
  sudo make install
else
  make install
fi

# Clean up
rm -rf $temp_dir
