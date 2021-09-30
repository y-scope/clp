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

package_name=libmariadb-dev

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
  # Get OS version
  source /etc/os-release
  if [ $ID = "ubuntu" ] ; then
    os_version=ubuntu-$UBUNTU_CODENAME
  elif [ $ID = "centos" ] ; then
    os_version=centos${VERSION_ID}
  fi

  # Download
  cd $temp_dir
  extracted_dir=${temp_dir}/mariadb-connector-c-${version}-${os_version}-amd64
  if [ ! -e ${extracted_dir} ] ; then
    tar_filename=mariadb-connector-c-${version}-${os_version}-amd64.tar.gz
    if [ ! -e ${tar_filename} ] ; then
      wget https://downloads.mariadb.com/Connectors/c/connector-c-${version}/${tar_filename}
    fi

    tar -xf ${tar_filename}
  fi

  cd ${extracted_dir}

  # Check if checkinstall is installed
  set +e
  command -v checkinstall
  checkinstall_installed=$?
  set -e

  # Install
  install_dir=/usr/local
  if [ $checkinstall_installed -ne 0 ] ; then
    # checkinstall is not installed, so install without building a deb package
    if [ ${EUID:-$(id -u)} -ne 0 ] ; then
      sudo mkdir -p ${install_dir}
      sudo rsync -a . ${install_dir}/
    else
      mkdir -p ${install_dir}
      rsync -a . ${install_dir}/
    fi
  else
    if [ ${EUID:-$(id -u)} -ne 0 ] ; then
      sudo mkdir -p ${install_dir}
      sudo checkinstall --pkgname "${package_name}" --pkgversion "${version}" --provides "${package_name}" --nodoc -y rsync -a . ${install_dir}/
    else
      mkdir -p ${install_dir}
      checkinstall --pkgname "${package_name}" --pkgversion "${version}" --provides "${package_name}" --nodoc -y rsync -a . ${install_dir}/
    fi
  fi
fi

# Clean up
rm -rf $temp_dir
