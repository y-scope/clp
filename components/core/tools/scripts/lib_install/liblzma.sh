#!/usr/bin/env bash

# Dependencies:
# - curl
# - make
# - gcc
# NOTE: Dependencies should be installed outside the script to allow the script to be largely distro-agnostic

set -o errexit
set -o nounset
set -o pipefail

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

# Ensure version must be greater or equal to 5.8.1 to mitigate both CVE-2024-3094 (resolved in
# version >5.6.1) and CVE-2025-31115 (resolved in version >5.8.0).
validate_minimum_required_version() {
    min_required_major=5
    min_required_minor=8
    min_required_patch=1

    local major minor patch

    IFS='.' read -r major minor patch <<< "$version"

    # Check the major version
    if (( major > min_required_major )); then
        return 0
    elif (( major < min_required_major )); then
        return 1
    fi

    # Check the minor version
    if (( minor > min_required_minor )); then
        return 0
    elif (( minor < min_required_minor )); then
        return 1
    fi

    # Check the patch version
    if (( patch >= min_required_patch )); then
        return 0
    fi

    return 1
}

if ! validate_minimum_required_version "$version"; then
    echo "Error: Version $version must be greater or equal to 5.8.1 to mitigate CVE-2024-3094 and" \
         " CVE-2025-31115."
    exit 1
fi

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
