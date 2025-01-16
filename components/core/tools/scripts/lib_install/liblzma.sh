#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

# Dependencies:
# - curl
# - make
# - gcc
# NOTE: Dependencies should be installed outside the script to allow the script to be largely distro-agnostic

for cmd in curl make gcc; do
    if ! $cmd --version >/dev/null 2>&1; then
        echo "Error: Required dependency '$cmd' not found"
        exit 1
    fi
done

cUsage="Usage: ${BASH_SOURCE[0]} <version>[ <.deb output directory>]"
if [ "$#" -lt 1 ] ; then
    echo $cUsage
    exit
fi
version=$1

package_name=liblzma
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
dpkg -l "${package_name}" | grep "${version}"
installed=$?
set -e
if [ $installed -eq 0 ] ; then
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
extracted_dir=${temp_dir}/xz-${version}
if [ ! -e ${extracted_dir} ] ; then
  tar_filename=xz-${version}.tar.gz
  if [ ! -e ${tar_filename} ] ; then
    curl -fsSL https://github.com/tukaani-project/xz/releases/download/v${version}/${tar_filename} -o ${tar_filename}
  fi
  tar -xf ${tar_filename}
fi

# Build
cd ${extracted_dir}
mkdir build
cd build
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ../
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
