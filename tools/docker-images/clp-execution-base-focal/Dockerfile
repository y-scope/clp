FROM ubuntu:focal AS BASE

WORKDIR /root

RUN mkdir -p ./tools/docker-images/clp-execution-base-focal
ADD ./tools/docker-images/clp-execution-base-focal/setup-scripts ./tools/docker-images/clp-execution-base-focal/setup-scripts

RUN ./tools/docker-images/clp-execution-base-focal/setup-scripts/install-prebuilt-packages.sh

# Remove cached files
RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Reset the working directory so that it's accessible by any user who runs the
# container
WORKDIR /

# Flatten the image
FROM scratch
COPY --from=BASE / /
