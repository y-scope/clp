FROM dokken/centos-stream-9 AS base

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
ADD ./tools/scripts/lib_install ./tools/scripts/lib_install

RUN ./tools/scripts/lib_install/centos-stream-9/install-all.sh

RUN pip3 install --no-cache-dir uv

# Remove cached files
RUN dnf clean all \
    && rm -rf /tmp/* /var/tmp/*

# Flatten the image
FROM scratch
COPY --from=base / /
