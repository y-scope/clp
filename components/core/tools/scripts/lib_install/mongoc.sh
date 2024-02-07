#!/usr/bin/env bash

# Dependencies:
# - cmake
# - curl
# - git
# - g++
# NOTE: Dependencies should be installed outside the script to allow the script to be largely
# distro-agnostic

# Exit on any error
set -e

# Error on undefined variable
set -u

cUsage="Usage: ${BASH_SOURCE[0]} <version>[ <.deb output directory>]"
if [ "$#" -lt 1 ] ; then
    echo "$cUsage"
    exit
fi
version=$1

package_name=libmongoc-dev
temp_dir="/tmp/${package_name}-installation"
deb_output_dir="${temp_dir}"
if [[ "$#" -gt 1 ]] ; then
  deb_output_dir="$(readlink -f "$2")"
  if [ ! -d "${deb_output_dir}" ] ; then
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

# Download
mkdir -p "$temp_dir"
cd "$temp_dir"
extracted_dir="${temp_dir}/mongo-c-driver-${version}"
if [ ! -e "${extracted_dir}" ] ; then
  tar_filename="mongo-c-driver-${version}.tar.gz"
  if [ ! -e "${tar_filename}" ] ; then
    curl \
      -fsSL \
      "https://github.com/mongodb/mongo-c-driver/releases/download/${version}/${tar_filename}" \
      -o "${tar_filename}"
  fi

  tar -xf "${tar_filename}"
fi

# Set up
cd "${extracted_dir}"
mkdir -p build
cd build
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF \
  -DENABLE_ICU=OFF \
  -DENABLE_TESTS=OFF \
  ..

# Check if checkinstall is installed
set +e
command -v checkinstall
checkinstall_installed=$?
set -e

# Install
if [ $checkinstall_installed -eq 0 ] ; then
  install_cmd_args+=(
    checkinstall
    --pkgname "${package_name}"
    --pkgversion "${version}"
    --provides "${package_name}"
    --nodoc
    -y
    --pakdir "${deb_output_dir}"
  )
fi
install_cmd_args+=(
  cmake
  --build .
  --target install
  --parallel
)
"${install_cmd_args[@]}"

# Clean up
rm -rf "$temp_dir"
