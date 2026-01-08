# syntax=docker/dockerfile:1

ARG QUAY_MIRROR=quay.io
FROM ${QUAY_MIRROR}/pypa/manylinux_2_28_aarch64

WORKDIR /root

ARG ALMALINUX_MIRROR
RUN if [ -n "${ALMALINUX_MIRROR}" ]; then \
        sed -i -e 's|^mirrorlist=|#mirrorlist=|g' \
               -e "s|^# baseurl=https://repo.almalinux.org|baseurl=https://${ALMALINUX_MIRROR}|g" \
               /etc/yum.repos.d/almalinux*.repo; \
    fi

RUN mkdir -p ./tools/scripts/lib_install
COPY --link ./tools/scripts/lib_install ./tools/scripts/lib_install

RUN pipx uninstall cmake
RUN pipx uninstall uv

RUN ./tools/scripts/lib_install/manylinux_2_28/install-all.sh

# Remove cached files
RUN dnf clean all && rm -rf /var/cache/dnf /tmp/* /var/tmp/*

# NOTE: Don't flatten the image or else we'll lose any environment modifications from the base
# image.
