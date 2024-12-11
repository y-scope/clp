#!/usr/bin/env bash

# Exit on any error
set -e

# Error on undefined variable
set -u

dnf install -y \
    cmake \
    diffutils \
    gcc-c++ \
    git \
    java-11-openjdk \
    jq \
    libarchive-devel \
    libcurl-devel \
    libzstd-devel \
    make \
    mariadb-connector-c-devel \
    openssl-devel \
    xz-devel
