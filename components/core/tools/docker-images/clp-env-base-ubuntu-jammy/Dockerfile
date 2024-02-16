FROM ubuntu:jammy AS BASE

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
ADD ./tools/scripts/lib_install ./tools/scripts/lib_install

RUN ./tools/scripts/lib_install/ubuntu-jammy/install-all.sh

# Remove cached files
RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Reset the working directory so that it's accessible by any user who runs the
# container
WORKDIR /

# Flatten the image
FROM scratch
COPY --from=BASE / /
