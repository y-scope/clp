FROM quay.io/pypa/manylinux_2_28_x86_64:2025.06.25-1

# Binaries built on manylinux_2_28 (AlmaLinux 8 based) is expected to be
# compatible with other distros using glibc 2.28 or later, including:
# - Debian 10+
# - Ubuntu 18.10+
# - Fedora 29+
# - CentOS/RHEL 8+

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
COPY ./tools/scripts/lib_install ./tools/scripts/lib_install

RUN ./tools/scripts/lib_install/manylinux_2_28/install-all.sh

# Remove cached files
RUN dnf clean all && rm -rf /var/cache/dnf /tmp/* /var/tmp/*

# NOTE: Don't flatten the image or else we'll lose any environment modifications from the base
# image.
