#!/bin/bash

# Dependencies:
# - curl
# - rsync
# NOTE: Dependencies should be installed outside the script to allow the script to be largely distro-agnostic

# Exit on any error
set -e

cUsage="Usage: ${BASH_SOURCE[0]} <version>[ <.deb output directory>]"
if [ "$#" -lt 1 ] ; then
    echo $cUsage
    exit
fi
version=$1

package_name=libmariadb-dev
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
dpkg -l ${package_name} | grep ${version}
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
  privileged_command_prefix="sudo"
  install_cmd_args+=(${privileged_command_prefix})
fi

# Get OS version
source /etc/os-release
if [ $ID = "ubuntu" ] ; then
  os_version=ubuntu-$UBUNTU_CODENAME
else
  echo "Unsupported OS ID: $ID"
  exit 1
fi

# Download
mkdir -p $temp_dir
cd $temp_dir
extracted_dir=${temp_dir}/mariadb-connector-c-${version}-${os_version}-amd64
if [ ! -e ${extracted_dir} ] ; then
  tar_filename=mariadb-connector-c-${version}-${os_version}-amd64.tar.gz
  if [ ! -e ${tar_filename} ] ; then
    curl -fsSL https://downloads.mariadb.com/Connectors/c/connector-c-${version}/${tar_filename} -o ${tar_filename}
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
${privileged_command_prefix} mkdir -p ${install_dir}

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
  rsync
  -a .
  "${install_dir}/"
)
"${install_cmd_args[@]}"

# Update ld cache
${privileged_command_prefix} ldconfig ${install_dir}/lib/mariadb

# Clean up
rm -rf $temp_dir
