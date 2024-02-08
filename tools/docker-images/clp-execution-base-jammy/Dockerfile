FROM ubuntu:jammy

WORKDIR /root

RUN mkdir -p ./tools/docker-images/clp-execution-base-jammy
ADD ./tools/docker-images/clp-execution-base-jammy/setup-scripts ./tools/docker-images/clp-execution-base-jammy/setup-scripts

RUN mkdir -p ./tools/scripts/lib_install
ADD ./components/core/tools/scripts/lib_install ./tools/scripts/lib_install

RUN ./tools/docker-images/clp-execution-base-jammy/setup-scripts/install-prebuilt-packages.sh

# Reset the working directory so that it's accessible by any user who runs the
# container
WORKDIR /
