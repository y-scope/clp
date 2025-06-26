#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

dnf install -y \
    gcc-c++ \
    java-11-openjdk \
    libcurl-devel \
    mariadb-connector-c-devel \
    openssl-devel \
    zlib-devel \
    zlib-static \
    jq

# Determine architecture for `task` release to install
rpm_arch=$(rpm --eval "%{_arch}")
case "$rpm_arch" in
    "x86_64")
        task_pkg_arch="amd64"
        ;;
    "aarch64")
        task_pkg_arch="arm64"
        ;;
    *)
        echo "Error: Unsupported architecture - $rpm_arch"
        exit 1
        ;;
esac

# Install `task`
# NOTE: We lock `task` to a version < 3.43 to avoid https://github.com/y-scope/clp/issues/872
task_pkg_path="$(mktemp -t --suffix ".rpm" task-pkg.XXXXXXXXXX)"
curl \
    --fail \
    --location \
    --output "$task_pkg_path" \
    --show-error \
    "https://github.com/go-task/task/releases/download/v3.42.1/task_linux_${task_pkg_arch}.rpm"
dnf install --assumeyes "$task_pkg_path"
rm "$task_pkg_path"

# The bundled CMake 4.x version is too new; downgrading to CMake 3.27 is
# necessary to resolve build issues with third-party libraries. Even after
# we added the "-DCMAKE_POLICY_VERSION_MINIMUM=3.5" flag, compilation issues
# in libraries such as googletest-src and yaml-cpp still persist.
# See sample error below:
# CMake Error at _deps/googletest-src/CMakeLists.txt:4 (cmake_minimum_required):
#   Support for CMake versions below 3.5 has been dropped.
#
#   Adjust the VERSION argument’s minimum value, or use the <min>...<max> syntax
#   to indicate the required CMake version range.
#
#   Alternatively, you can try adding -DCMAKE_POLICY_VERSION_MINIMUM=3.5
export PLATFORM=$(uname -m)
export CMAKE_VERSION=3.27.9
export CMAKE_RELEASE_URL=https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-${PLATFORM}.tar.gz
echo "Downgrading CMAKE to 3.27.9"
curl -L ${CMAKE_RELEASE_URL} | tar -xzf -
mv cmake-${CMAKE_VERSION}-linux-${PLATFORM} /opt/cmake-${CMAKE_VERSION}
ln -sf /opt/cmake-${CMAKE_VERSION}/bin/cmake /usr/local/bin/cmake
