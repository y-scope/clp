#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

dnf install -y \
    cmake \
    gcc-c++ \
    git \
    java-11-openjdk \
    libarchive-devel \
    libcurl-devel \
    libzstd-devel \
    make \
    mariadb-connector-c-devel \
    openssl-devel
