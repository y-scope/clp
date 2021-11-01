FROM ubuntu:focal

WORKDIR /root

RUN mkdir -p ./tools/docker-images/clp-execution-base-focal
ADD ./tools/docker-images/clp-execution-base-focal/setup-scripts ./tools/docker-images/clp-execution-base-focal/setup-scripts

RUN mkdir -p ./tools/scripts/lib_install
ADD ./components/core/tools/scripts/lib_install ./tools/scripts/lib_install

RUN ./tools/docker-images/clp-execution-base-focal/setup-scripts/install-prebuilt-packages.sh
RUN ./tools/docker-images/clp-execution-base-focal/setup-scripts/install-packages-from-source.sh
