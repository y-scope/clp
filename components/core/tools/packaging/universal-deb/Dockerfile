# syntax=docker/dockerfile:1
#
# Extends the CLP manylinux_2_28 build image with tools needed for packaging:
# patchelf (to rewrite RPATH), dpkg (for .deb), and rpm-build (for .rpm).
# Installing all packaging tools in one image allows Docker to cache the
# shared layers between deb and rpm builds.
#
# Usage:
#   docker build --build-arg BASE_IMAGE=clp-core-dependencies-x86-manylinux_2_28:dev ...

ARG BASE_IMAGE
FROM ${BASE_IMAGE}

RUN dnf install -y epel-release \
    && dnf install -y dpkg patchelf rpm-build \
    && dnf clean all
