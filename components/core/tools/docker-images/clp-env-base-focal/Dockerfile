FROM ubuntu:focal

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
ADD ./tools/scripts/lib_install ./tools/scripts/lib_install

RUN ./tools/scripts/lib_install/ubuntu-focal/install-all.sh

# Set the compiler to gcc-10
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 10
RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 10
RUN update-alternatives --set gcc /usr/bin/gcc-10
RUN update-alternatives --set g++ /usr/bin/g++-10

# Reset the working directory so that it's accessible by any user who runs the
# container
WORKDIR /
