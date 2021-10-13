FROM ubuntu:focal

RUN mkdir -p /root/tools/docker-images/clp-execution-base-focal
ADD ./tools/docker-images/clp-execution-base-focal/setup-scripts /root/tools/docker-images/clp-execution-base-focal/setup-scripts

RUN mkdir -p /root/tools/scripts/lib_install
ADD ./components/core/tools/scripts/lib_install /root/tools/scripts/lib_install

RUN cd /root && ./tools/docker-images/clp-execution-base-focal/setup-scripts/install-prebuilt-packages.sh
RUN cd /root && ./tools/docker-images/clp-execution-base-focal/setup-scripts/install-packages-from-source.sh
