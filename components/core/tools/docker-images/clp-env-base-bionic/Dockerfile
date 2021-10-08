FROM ubuntu:bionic

RUN mkdir -p /root/tools/docker-images/clp-env-base-bionic
ADD ./tools/docker-images/clp-env-base-bionic/setup-scripts /root/tools/docker-images/clp-env-base-bionic/setup-scripts

RUN mkdir -p /root/tools/scripts/lib_install
ADD ./tools/scripts/lib_install /root/tools/scripts/lib_install

RUN cd /root && ./tools/docker-images/clp-env-base-bionic/setup-scripts/install-prebuilt-packages.sh
RUN cd /root && ./tools/docker-images/clp-env-base-bionic/setup-scripts/install-packages-from-source.sh
