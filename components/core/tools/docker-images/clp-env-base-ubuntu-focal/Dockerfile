FROM ubuntu:focal AS BASE

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
ADD ./tools/scripts/lib_install ./tools/scripts/lib_install

RUN ./tools/scripts/lib_install/ubuntu-focal/install-all.sh

# Set gcc/g++ 10 as the default compiler
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 10 \
    && update-alternatives --set gcc /usr/bin/gcc-10 \
    && update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-10 20 \
    && update-alternatives --set cc /usr/bin/gcc-10 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 10 \
    && update-alternatives --set g++ /usr/bin/g++-10 \
    && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-10 20 \
    && update-alternatives --set c++ /usr/bin/g++-10

# Remove cached files
RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Reset the working directory so that it's accessible by any user who runs the
# container
WORKDIR /

# Flatten the image
FROM scratch
COPY --from=BASE / /
