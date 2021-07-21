#!/bin/bash

# Exit on any error
set -e

echo "Checking for elevated privileges..."
if [ ${EUID:-$(id -u)} -ne 0 ] ; then
  sudo echo "Script can elevate privileges."
fi

# Get number of cpu cores
num_cpus=$(grep -c ^processor /proc/cpuinfo)

# Create temp dir for installation
temp_dir=/tmp/zstandard-installation
mkdir -p $temp_dir

# Check if already installed
package_name=libzstd
set +e
dpkg -l ${package_name}
installed=$?
set -e

# Install if necessary
version=1.4.9
if [ $installed -ne 0 ] ; then
  # Download and build
  cd $temp_dir
  extracted_dir=${temp_dir}/zstd-${version}
  if [ ! -e ${extracted_dir} ] ; then
    tar_filename=zstd-${version}.tar.gz
    if [ ! -e ${tar_filename} ] ; then
      wget https://github.com/facebook/zstd/releases/download/v${version}/${tar_filename}
    fi

    tar -xf ${tar_filename}
  fi

  cd ${extracted_dir}/build/cmake
  mkdir cmake-build-release
  cd cmake-build-release
  cmake ../
  make -j${num_cpus}

  # Install
  if [ ${EUID:-$(id -u)} -ne 0 ] ; then
    sudo checkinstall --pkgname "${package_name}" --pkgversion "${version}" --provides "${package_name}" --nodoc -y
  else
    checkinstall --pkgname "${package_name}" --pkgversion "${version}" --provides "${package_name}" --nodoc -y
  fi
fi

# Clean up
rm -rf $temp_dir
