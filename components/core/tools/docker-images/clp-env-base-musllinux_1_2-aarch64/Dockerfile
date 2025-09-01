FROM quay.io/pypa/musllinux_1_2_aarch64

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
COPY ./tools/scripts/lib_install ./tools/scripts/lib_install

RUN pipx uninstall cmake || true
RUN pipx uninstall go-task-bin || true
RUN pipx uninstall uv || true

RUN ./tools/scripts/lib_install/musllinux_1_2/install-all.sh

# Remove cached files
RUN apk cache clean --all && rm -rf /var/cache/apk/* /tmp/* /var/tmp/*
