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

package_name=libarchive

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
  extracted_dir=${temp_dir}/${package_name}-${version}
  if [ ! -e ${extracted_dir} ] ; then
    tar_filename=${package_name}-${version}.tar.gz
    if [ ! -e ${tar_filename} ] ; then
      wget https://www.libarchive.org/downloads/${tar_filename}
    fi

    tar -xf ${tar_filename}
  fi

  cd ${extracted_dir}
  mkdir -p cmake-build-release
  cd cmake-build-release
  # NOTE: Disable expat so the static libarchive doesn't look for it at link time
  cmake -DENABLE_EXPAT=OFF ../
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
