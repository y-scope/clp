FROM ubuntu:jammy AS BASE

WORKDIR /root

RUN mkdir -p ./tools/docker-images/clp-execution-base-ubuntu-jammy
ADD ./tools/docker-images/clp-execution-base-ubuntu-jammy/setup-scripts ./tools/docker-images/clp-execution-base-ubuntu-jammy/setup-scripts

RUN ./tools/docker-images/clp-execution-base-ubuntu-jammy/setup-scripts/install-prebuilt-packages.sh

# Remove cached files
RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Flatten the image
FROM scratch
COPY --from=BASE / /
