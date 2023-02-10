#!/usr/bin/env bash

# Exit on error
set -e

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

# Get number of cpu cores
num_cpus=$(grep -c ^processor /proc/cpuinfo)

package_name=cmake

# Create temp dir for installation
temp_dir=/tmp/${package_name}-installation
mkdir -p $temp_dir

cd $temp_dir

# Download source
tar_filename=cmake-${version}.tar.gz
curl -fsSL https://github.com/Kitware/CMake/releases/download/v${version}/${tar_filename} -o ${tar_filename}
tar xzf ${tar_filename}
cd cmake-${version}

# Build
./bootstrap
make -j${num_cpus}

# Install
if [ ${EUID:-$(id -u)} -ne 0 ] ; then
  sudo make install
else
  make install
fi

# Clean up
rm -rf $temp_dir
