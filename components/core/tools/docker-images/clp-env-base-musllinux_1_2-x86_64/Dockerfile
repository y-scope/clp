FROM quay.io/pypa/musllinux_1_2_x86_64 AS base

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
COPY ./tools/scripts/lib_install ./tools/scripts/lib_install

RUN ./tools/scripts/lib_install/musllinux_1_2/install-all.sh

# Remove cached files
RUN apk cache clean --all && rm -rf /var/cache/apk/* /tmp/* /var/tmp/*
