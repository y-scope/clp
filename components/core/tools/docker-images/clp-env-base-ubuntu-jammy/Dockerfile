# syntax=docker/dockerfile:1

FROM ubuntu:jammy AS base

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
COPY --link ./tools/scripts/lib_install ./tools/scripts/lib_install

# Set pipx environment vars so that the pipx scripts install into the global locations without
# passing the `--global` flag.
ENV PIPX_BIN_DIR=/usr/local/bin
ENV PIPX_HOME=/opt/pipx

RUN ./tools/scripts/lib_install/ubuntu-jammy/install-all.sh

# Remove cached files
RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Flatten the image
FROM scratch
COPY --link --from=base / /
