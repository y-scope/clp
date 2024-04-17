FROM ubuntu:focal AS BASE

WORKDIR /root

RUN mkdir -p ./tools/docker-images/clp-execution-base-ubuntu-focal
ADD ./tools/docker-images/clp-execution-base-ubuntu-focal/setup-scripts ./tools/docker-images/clp-execution-base-ubuntu-focal/setup-scripts

RUN ./tools/docker-images/clp-execution-base-ubuntu-focal/setup-scripts/install-prebuilt-packages.sh

# Remove cached files
RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Flatten the image
FROM scratch
COPY --from=BASE / /
