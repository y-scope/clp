FROM ubuntu:jammy AS BASE

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
ADD ./tools/scripts/lib_install ./tools/scripts/lib_install

RUN ./tools/scripts/lib_install/ubuntu-jammy/install-all.sh

# Install Task
RUN sh -c "$(curl --location https://taskfile.dev/install.sh)" -- -d

# Remove cached files
RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Flatten the image
FROM scratch
COPY --from=BASE / /
