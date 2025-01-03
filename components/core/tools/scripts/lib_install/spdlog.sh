#!/bin/bash

# Dependencies:
# - cmake
# - curl
# - g++
# NOTE: Dependencies should be installed outside the script to allow the script to be largely distro-agnostic

# Exit on any error
set -e

cUsage="Usage: ${BASH_SOURCE[0]} <version>[ <.deb output directory>]"
if [ "$#" -lt 1 ] ; then
    echo $cUsage
    exit
fi
version=$1

lib_name=spdlog
package_name=libspdlog-dev
temp_dir=/tmp/${package_name}-installation
deb_output_dir=${temp_dir}
if [[ "$#" -gt 1 ]] ; then
  deb_output_dir="$(readlink -f "$2")"
  if [ ! -d ${deb_output_dir} ] ; then
    echo "${deb_output_dir} does not exist or is not a directory"
    exit
  fi
fi

# Check if already installed
set +e
pkg-config --exact-version="${version}" "${lib_name}"
pkg_found=$?
if [ $pkg_found -eq 0 ] ; then
  find /usr/lib/ /usr/local/lib/ -name "libspdlog.a" | grep -q "."
  static_lib_found=$?
fi
installed=$((pkg_found | static_lib_found))
set -e
if [ $installed -eq 0 ] ; then
  echo "Found ${lib_name}=${version}."
  # Nothing to do
  exit
fi

echo "Checking for elevated privileges..."
install_cmd_args=()
if [ ${EUID:-$(id -u)} -ne 0 ] ; then
  sudo echo "Script can elevate privileges."
  install_cmd_args+=("sudo")
fi

# Get number of cpu cores
num_cpus=$(grep -c ^processor /proc/cpuinfo)

# Download
mkdir -p $temp_dir
cd $temp_dir
extracted_dir=${temp_dir}/spdlog-${version}
if [ ! -e ${extracted_dir} ] ; then
  tar_filename=v${version}.tar.gz
  if [ ! -e ${tar_filename} ] ; then
    curl -fsSL https://github.com/gabime/spdlog/archive/${tar_filename} -o ${tar_filename}
  fi

  tar -xf ${tar_filename}
fi

# Build
cd ${extracted_dir}
mkdir -p build
cd build
cmake ../
make -j${num_cpus}

# Check if checkinstall is installed
set +e
command -v checkinstall
checkinstall_installed=$?
set -e

# Install
if [ $checkinstall_installed -eq 0 ] ; then
  install_cmd_args+=(
    checkinstall
    --default
    --fstrans=no
    --nodoc
    --pkgname "${package_name}"
    --pkgversion "${version}"
    --provides "${package_name}"
    --pakdir "${deb_output_dir}"
  )
fi
install_cmd_args+=(
  make install
)
"${install_cmd_args[@]}"

# Clean up
rm -rf $temp_dir
