FROM ubuntu:focal

WORKDIR /root

RUN mkdir -p ./tools/docker-images/clp-execution-base-focal
ADD ./tools/docker-images/clp-execution-base-focal/setup-scripts ./tools/docker-images/clp-execution-base-focal/setup-scripts

RUN ./tools/docker-images/clp-execution-base-focal/setup-scripts/install-prebuilt-packages.sh

# Reset the working directory so that it's accessible by any user who runs the
# container
WORKDIR /
