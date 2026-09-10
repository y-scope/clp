# syntax=docker/dockerfile:1
#
# Extends the CLP manylinux_2_28 build image with tools needed for packaging:
# patchelf (to rewrite RPATH), dpkg (for .deb), and rpm-build (for .rpm).
# Installing all packaging tools in one image allows Docker to cache the
# shared layers between deb and rpm builds.
#
# Usage:
#   docker build --build-arg BASE_IMAGE=clp-core-dependencies-x86-manylinux_2_28:dev ...

# Declared before the first FROM so it is a meta-ARG, usable in `FROM ${BASE_IMAGE}`
# below. Declaring it after the ca_trust stage would scope it to that stage and the
# base name would resolve to empty.
ARG BASE_IMAGE

# Empty default for the `ca_trust` build context, so the networked RUN below resolves its mount when
# no host CA trust is supplied. `build.sh --with-ca-certs` overrides this stage with a staged
# directory. Nothing is ever baked in. See yscope-dev-utils exports/docker/ca-trust.
FROM scratch AS ca_trust

FROM ${BASE_IMAGE}

# ca-trust-pkg-opts.sh below is bash: it declares CLP_DNF_CA_OPTS as an array. The RUN would
# otherwise run under the base image's /bin/sh, which happens to be bash on the AlmaLinux-based
# manylinux image but is dash or busybox elsewhere -- where sourcing the script is a syntax error.
SHELL ["/bin/bash", "-c"]

# dnf reads neither SSL_CERT_FILE nor CURL_CA_BUNDLE, so ca-trust-pkg-opts.sh supplies its
# --setopt=sslcacert explicitly; the env vars container.sh exports cover curl and pip only.
RUN --mount=type=bind,from=ca_trust,target=/run/ca-trust \
    if [ -e /run/ca-trust/container.sh ]; then \
        export CA_TRUST_DIR=/run/ca-trust; . /run/ca-trust/container.sh; \
    fi \
    && . ./tools/scripts/lib_install/ca-trust-pkg-opts.sh \
    && dnf "${CLP_DNF_CA_OPTS[@]}" install -y epel-release \
    && dnf "${CLP_DNF_CA_OPTS[@]}" install -y dpkg patchelf rpm-build \
    # `dnf install dpkg` pulls in AlmaLinux's python3-3.6, which registers /usr/bin/python3
    # via alternatives at priority 1000000 and so wins over the manylinux Python, leaving
    # `python3` resolving to 3.6. That's too old for build-and-run-unit-tests.py (uses
    # `from __future__ import annotations`, 3.7+). Re-register the manylinux
    # /usr/local/bin/python3.12 at a higher priority to restore it as the default `python3`.
    && update-alternatives --install /usr/bin/python3 python3 /usr/local/bin/python3.12 2000000 \
    && dnf clean all
