# syntax=docker/dockerfile:1
#
# Extends the CLP musllinux_1_2 build image with tools needed for .apk
# packaging: patchelf (to rewrite RPATH) and alpine-sdk (for abuild).
#
# Usage:
#   docker build --build-arg BASE_IMAGE=clp-core-dependencies-x86-musllinux_1_2:dev ...

# Declared before the first FROM so it is a meta-ARG, usable in `FROM ${BASE_IMAGE}`
# below. Declaring it after the ca_trust stage would scope it to that stage and the
# base name would resolve to empty.
ARG BASE_IMAGE

# Empty default for the `ca_trust` build context, so the networked RUN below resolves its mount when
# no host CA trust is supplied. `build.sh --with-ca-certs` overrides this stage with a staged
# directory. Nothing is ever baked in. See yscope-dev-utils exports/docker/ca-trust.
FROM scratch AS ca_trust

FROM ${BASE_IMAGE}

RUN --mount=type=bind,from=ca_trust,target=/run/ca-trust \
    if [ -e /run/ca-trust/container.sh ]; then \
        export CA_TRUST_DIR=/run/ca-trust; runner="bash /run/ca-trust/container-exec.sh"; \
    else runner=""; fi; \
    $runner apk add --no-cache patchelf alpine-sdk
