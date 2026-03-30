# syntax=docker/dockerfile:1

FROM quay.io/pypa/musllinux_1_2_x86_64 AS base

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
COPY --link ./tools/scripts/lib_install ./tools/scripts/lib_install
COPY --link ./tools/scripts/corporate-proxy-container.sh ./tools/scripts/corporate-proxy-container.sh
COPY --link ./tools/scripts/ca-certificates.crt ./tools/scripts/ca-certificates.crt
# Corporate proxy support: install host CA certificates into the container's
# trust store so tools like curl and apk can verify TLS through intercepting
# proxies (e.g., Zscaler, Fortinet, Palo Alto). See corporate-proxy-host.sh
# for details.
RUN ./tools/scripts/corporate-proxy-container.sh

# Point pip/pipx and curl at the corporate CA bundle path set by corporate-proxy-container.sh.
# This path is outside the system package manager's control, so it survives reinstalls of
# ca-certificates (which regenerates the system bundle). When no corporate proxy is in use,
# this path won't exist; tools fall back to their built-in defaults which work fine without a proxy.
ENV CURL_CA_BUNDLE=/opt/corp-ca/ca-bundle.crt \
    PIP_CERT=/opt/corp-ca/ca-bundle.crt \
    REQUESTS_CA_BUNDLE=/opt/corp-ca/ca-bundle.crt \
    SSL_CERT_FILE=/opt/corp-ca/ca-bundle.crt

# Optional: override Alpine mirrors (e.g., organization-internal or regional).
# Usage: APK_MIRROR_URL=<url> ./build.sh
ARG APK_MIRROR_URL
RUN if [ -n "${APK_MIRROR_URL:-}" ]; then \
        sed -i "s|https://dl-cdn.alpinelinux.org/alpine|${APK_MIRROR_URL}|g" /etc/apk/repositories && \
        apk update; \
    fi

RUN pipx uninstall cmake
RUN pipx uninstall uv

RUN ./tools/scripts/lib_install/musllinux_1_2/install-all.sh || { \
        echo ""; \
        echo "==============================================================="; \
        echo "ERROR: Package installation failed."; \
        echo ""; \
        echo "If the error above mentions download failures, timeouts, or"; \
        echo "unreachable mirrors, try overriding with a different mirror:"; \
        echo "  APK_MIRROR_URL=<url> ./build.sh"; \
        echo ""; \
        echo "Use your organization's internal mirror or pick one from:"; \
        echo "  https://mirrors.alpinelinux.org/"; \
        echo "==============================================================="; \
        exit 1; \
    }

# Remove cached files
RUN apk cache clean --all && rm -rf /var/cache/apk/* /tmp/* /var/tmp/*
