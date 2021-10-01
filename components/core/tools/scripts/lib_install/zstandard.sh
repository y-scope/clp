#!/bin/bash

# Exit on any error
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

package_name=libzstd

# Create temp dir for installation
temp_dir=/tmp/${package_name}-installation
mkdir -p $temp_dir

# Check if already installed
set +e
dpkg -l ${package_name}
installed=$?
set -e

# Install if necessary
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

  # Check if checkinstall is installed
  set +e
  command -v checkinstall
  checkinstall_installed=$?
  set -e

  # Install
  if [ $checkinstall_installed -ne 0 ] ; then
    # checkinstall is not installed, so install without building a deb package
    if [ ${EUID:-$(id -u)} -ne 0 ] ; then
      sudo make install
    else
      make install
    fi
  else
    if [ ${EUID:-$(id -u)} -ne 0 ] ; then
      sudo checkinstall --pkgname "${package_name}" --pkgversion "${version}" --provides "${package_name}" --nodoc -y
    else
      checkinstall --pkgname "${package_name}" --pkgversion "${version}" --provides "${package_name}" --nodoc -y
    fi
  fi
fi

# Clean up
rm -rf $temp_dir
