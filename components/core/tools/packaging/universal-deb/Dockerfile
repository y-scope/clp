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
    # `dnf install dpkg` pulls in AlmaLinux's python3-3.6, which registers /usr/bin/python3
    # via alternatives at priority 1000000 and so wins over the manylinux Python, leaving
    # `python3` resolving to 3.6. That's too old for build-and-run-unit-tests.py (uses
    # `from __future__ import annotations`, 3.7+). Re-register the manylinux
    # /usr/local/bin/python3.12 at a higher priority to restore it as the default `python3`.
    && update-alternatives --install /usr/bin/python3 python3 /usr/local/bin/python3.12 2000000 \
    && dnf clean all
