FROM ubuntu:noble AS base

WORKDIR /root

RUN mkdir -p ./tools/docker-images/clp-execution-base-ubuntu-noble
COPY ./tools/docker-images/clp-execution-base-ubuntu-noble/setup-scripts ./tools/docker-images/clp-execution-base-ubuntu-noble/setup-scripts

RUN ./tools/docker-images/clp-execution-base-ubuntu-noble/setup-scripts/install-prebuilt-packages.sh

# Remove cached files
RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Flatten the image
FROM scratch
COPY --from=base / /
