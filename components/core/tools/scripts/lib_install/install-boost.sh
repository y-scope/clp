#!/usr/bin/env bash

# Exit on error
set -e

cUsage="Usage: ${BASH_SOURCE[0]} <version>"
if [ "$#" -lt 1 ] ; then
    echo $cUsage
    exit
fi
version=$1
version_with_underscores=${version//./_}

echo "Checking for elevated privileges..."
if [ ${EUID:-$(id -u)} -ne 0 ] ; then
  sudo echo "Script can elevate privileges."
fi

# Get number of cpu cores
num_cpus=$(grep -c ^processor /proc/cpuinfo)

package_name=boost

# Create temp dir for installation
temp_dir=/tmp/${package_name}-installation
mkdir -p $temp_dir

cd $temp_dir

# Download source
tar_filename=boost_${version_with_underscores}.tar.gz
curl -fsSL https://archives.boost.io/release/${version}/source/${tar_filename} -o ${tar_filename}
tar xzf ${tar_filename}
cd boost_${version_with_underscores}

# Build
./bootstrap.sh --with-libraries=filesystem,iostreams,program_options,regex,system,url
./b2 -j${num_cpus}

# Install
if [ ${EUID:-$(id -u)} -ne 0 ] ; then
  sudo ./b2 install
else
  ./b2 install
fi

# Clean up
rm -rf $temp_dir
