FROM dokken/centos-stream-9 AS base

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
ADD ./tools/scripts/lib_install ./tools/scripts/lib_install

RUN ./tools/scripts/lib_install/centos-stream-9/install-all.sh

# Install uv and create a cache dir for its use
RUN pip3 install --no-cache-dir uv==0.8.3
RUN mkdir -p /tmp/.cache

# Remove cached files
RUN dnf clean all \
    && rm -rf /tmp/* /var/tmp/*

# Flatten the image
FROM scratch
COPY --from=base / /
