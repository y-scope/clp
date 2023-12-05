FROM ubuntu:focal

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
ADD ./tools/scripts/lib_install ./tools/scripts/lib_install

RUN ./tools/scripts/lib_install/ubuntu-focal/install-all.sh

# Reset the working directory so that it's accessible by any user who runs the
# container
WORKDIR /
