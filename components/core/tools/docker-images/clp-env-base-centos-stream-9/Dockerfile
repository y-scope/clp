FROM dokken/centos-stream-9 AS base

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
ADD ./tools/scripts/lib_install ./tools/scripts/lib_install

RUN ./tools/scripts/lib_install/centos-stream-9/install-all.sh

# NOTE:
# 1. `task` doesn't have an apt/dnf package so we use its install script.
# 2. We don't want to install it using `install-prebuilt-packages.sh` since users may use that on
#    their own machines and it would change their environment in a way that can't easily be undone.
RUN sh -c "$(curl --location https://taskfile.dev/install.sh)" -- -d -b /usr/local/bin

# Remove cached files
RUN dnf clean all \
    && rm -rf /tmp/* /var/tmp/*

# Flatten the image
FROM scratch
COPY --from=base / /
