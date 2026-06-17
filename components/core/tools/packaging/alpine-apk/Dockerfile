# syntax=docker/dockerfile:1
#
# Extends the CLP musllinux_1_2 build image with the tools required for
# .apk packaging: patchelf for RPATH rewriting, alpine-sdk for abuild,
# and the SBOM toolchain (syft for OS-package inventory, python3 +
# PyYAML for the manifest-side merger).
#
# Usage:
#   docker build --build-arg BASE_IMAGE=clp-core-dependencies-x86-musllinux_1_2:dev ...

ARG BASE_IMAGE
FROM ${BASE_IMAGE}

# syft is the OS-package scanner used by common/generate-sbom.sh. The
# version pinned here must match universal-deb/Dockerfile.
ARG SYFT_VERSION=v1.45.0

RUN apk add --no-cache binutils patchelf alpine-sdk python3 py3-yaml curl \
    && curl -sSfL https://raw.githubusercontent.com/anchore/syft/${SYFT_VERSION}/install.sh \
       | sh -s -- -b /usr/local/bin "${SYFT_VERSION}"
