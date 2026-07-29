# syntax=docker/dockerfile:1

FROM ubuntu:jammy AS base

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
COPY --link ./tools/scripts/lib_install ./tools/scripts/lib_install
COPY --link ./tools/scripts/corporate-proxy-container.sh ./tools/scripts/corporate-proxy-container.sh
COPY --link ./tools/scripts/ca-certificates.crt ./tools/scripts/ca-certificates.crt
# Corporate proxy support: install host CA certificates into the container's
# trust store so tools like curl and apt can verify TLS through intercepting
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

# Optional: override Ubuntu mirrors (e.g., organization-internal or regional).
# Usage: APT_MIRROR_URL=<url> ./build.sh
# Handles both x86 (archive.ubuntu.com/ubuntu) and ARM (ports.ubuntu.com/ubuntu-ports).
ARG APT_MIRROR_URL
RUN if [ -n "${APT_MIRROR_URL:-}" ]; then \
        sed -i \
            -e "s|https\?://ports.ubuntu.com/ubuntu-ports|${APT_MIRROR_URL}|g" \
            -e "s|https\?://archive.ubuntu.com/ubuntu|${APT_MIRROR_URL}|g" \
            -e "s|https\?://security.ubuntu.com/ubuntu|${APT_MIRROR_URL}|g" \
            /etc/apt/sources.list && \
        apt-get update; \
    fi

# Set pipx environment vars so that the pipx scripts install into the global locations without
# passing the `--global` flag.
ENV PIPX_BIN_DIR=/usr/local/bin
ENV PIPX_HOME=/opt/pipx

RUN ./tools/scripts/lib_install/ubuntu-jammy/install-all.sh || { \
        echo ""; \
        echo "==============================================================="; \
        echo "ERROR: Package installation failed."; \
        echo ""; \
        echo "If the error above mentions download failures, timeouts, or"; \
        echo "unreachable mirrors, try overriding with a different mirror:"; \
        echo "  APT_MIRROR_URL=<url> ./build.sh"; \
        echo ""; \
        echo "Use your organization's internal mirror or pick one from:"; \
        echo "  https://launchpad.net/ubuntu/+archivemirrors"; \
        echo "==============================================================="; \
        exit 1; \
    }

# Remove cached files
RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Flatten the image
FROM scratch
COPY --link --from=base / /
ENV CURL_CA_BUNDLE=/opt/corp-ca/ca-bundle.crt \
    PIP_CERT=/opt/corp-ca/ca-bundle.crt \
    REQUESTS_CA_BUNDLE=/opt/corp-ca/ca-bundle.crt \
    SSL_CERT_FILE=/opt/corp-ca/ca-bundle.crt
ENV PIPX_BIN_DIR=/usr/local/bin
ENV PIPX_HOME=/opt/pipx
