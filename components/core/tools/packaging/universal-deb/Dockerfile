# syntax=docker/dockerfile:1
#
# Extends the CLP manylinux_2_28 build image with the tools required for
# packaging: patchelf for RPATH rewriting, dpkg for .deb builds,
# rpm-build for .rpm builds, and the SBOM toolchain (syft for OS-package
# inventory, python3 + PyYAML for the manifest-side merger).
#
# Installing all packaging tools in one image lets Docker share cached
# layers between deb and rpm builds.
#
# Usage:
#   docker build --build-arg BASE_IMAGE=clp-core-dependencies-x86-manylinux_2_28:dev ...

ARG BASE_IMAGE
FROM ${BASE_IMAGE}

# syft is the OS-package scanner used by common/generate-sbom.sh. The
# version pinned here must match alpine-apk/Dockerfile.
ARG SYFT_VERSION=v1.45.0

RUN dnf install -y dpkg patchelf rpm-build python3 python3-pyyaml \
    && dnf clean all \
    && curl -sSfL https://raw.githubusercontent.com/anchore/syft/${SYFT_VERSION}/install.sh \
       | sh -s -- -b /usr/local/bin "${SYFT_VERSION}"
