FROM dokken/centos-stream-9 AS base

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
ADD ./tools/scripts/lib_install ./tools/scripts/lib_install

ENV PIPX_BIN_DIR=/usr/local/bin
ENV PIPX_HOME=/opt/_internal/pipx

RUN ./tools/scripts/lib_install/centos-stream-9/install-all.sh

# Remove cached files
RUN dnf clean all \
    && rm -rf /tmp/* /var/tmp/*

# Flatten the image
FROM scratch
COPY --from=base / /
