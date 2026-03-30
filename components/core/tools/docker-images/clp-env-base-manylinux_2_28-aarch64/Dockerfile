# syntax=docker/dockerfile:1

FROM quay.io/pypa/manylinux_2_28_aarch64

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
COPY --link ./tools/scripts/lib_install ./tools/scripts/lib_install
COPY --link ./tools/scripts/corporate-proxy-container.sh ./tools/scripts/corporate-proxy-container.sh
COPY --link ./tools/scripts/ca-certificates.crt ./tools/scripts/ca-certificates.crt
# Corporate proxy support: install host CA certificates into the container's
# trust store so tools like curl and dnf can verify TLS through intercepting
# proxies (e.g., Zscaler, Fortinet, Palo Alto). See corporate-proxy-host.sh
# for details.
RUN ./tools/scripts/corporate-proxy-container.sh

# Point pip/pipx and curl at the CA bundle installed by corporate-proxy-container.sh.
# This path is outside the system package manager's control, so it survives reinstalls of
# ca-certificates (which regenerates the system bundle).
ENV CURL_CA_BUNDLE=/opt/corp-ca/ca-bundle.crt \
    PIP_CERT=/opt/corp-ca/ca-bundle.crt \
    REQUESTS_CA_BUNDLE=/opt/corp-ca/ca-bundle.crt \
    SSL_CERT_FILE=/opt/corp-ca/ca-bundle.crt

# Optional: override AlmaLinux mirrors (e.g., organization-internal or regional).
# Usage: DNF_MIRROR_BASE_URL=<url> ./build.sh
ARG DNF_MIRROR_BASE_URL
RUN if [ -n "${DNF_MIRROR_BASE_URL:-}" ]; then \
        sed -i 's|^mirrorlist=|#mirrorlist=|g' /etc/yum.repos.d/almalinux*.repo && \
        sed -i "s|^# *baseurl=https\?://repo.almalinux.org/almalinux|baseurl=${DNF_MIRROR_BASE_URL}|g" /etc/yum.repos.d/almalinux*.repo && \
        dnf clean all && dnf makecache; \
    fi

RUN pipx uninstall cmake
RUN pipx uninstall uv

RUN ./tools/scripts/lib_install/manylinux_2_28/install-all.sh || { \
        echo ""; \
        echo "==============================================================="; \
        echo "ERROR: Package installation failed."; \
        echo ""; \
        echo "If the error above mentions download failures, timeouts, or"; \
        echo "unreachable mirrors, try overriding with a different mirror:"; \
        echo "  DNF_MIRROR_BASE_URL=<url> ./build.sh"; \
        echo ""; \
        echo "Use your organization's internal mirror or pick one from:"; \
        echo "  https://mirrors.almalinux.org/"; \
        echo "==============================================================="; \
        exit 1; \
    }

# Remove cached files
RUN dnf clean all && rm -rf /var/cache/dnf /tmp/* /var/tmp/*

# NOTE: Don't flatten the image or else we'll lose any environment modifications from the base
# image.
