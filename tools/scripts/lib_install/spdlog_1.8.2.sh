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
temp_dir=/tmp/spdlog-installation
mkdir -p $temp_dir

# Check if already installed
package_name=libspdlog-dev
set +e
dpkg -l ${package_name}
installed=$?
set -e

# Install if necessary
version=1.8.2
if [ $installed -ne 0 ] ; then
  # Download and build
  cd $temp_dir
  extracted_dir=${temp_dir}/spdlog-${version}
  if [ ! -e ${extracted_dir} ] ; then
    tar_filename=v${version}.tar.gz
    if [ ! -e ${tar_filename} ] ; then
      wget https://github.com/gabime/spdlog/archive/${tar_filename}
    fi

    tar -xf ${tar_filename}
  fi

  cd ${extracted_dir}
  mkdir -p build
  cd build
  cmake ../
  make -j${num_cpus}

  # Install
  # NOTE: We use "1:${version}" to override the version installed by Ubuntu 18.04
  if [ ${EUID:-$(id -u)} -ne 0 ] ; then
    sudo checkinstall --pkgname "${package_name}" --pkgversion "1:${version}" --provides "${package_name}" --nodoc -y
  else
    checkinstall --pkgname "${package_name}" --pkgversion "1:${version}" --provides "${package_name}" --nodoc -y
  fi
fi

# Clean up
rm -rf $temp_dir
