# syntax=docker/dockerfile:1

FROM quay.io/pypa/musllinux_1_2_aarch64

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
COPY --link ./tools/scripts/lib_install ./tools/scripts/lib_install
# Corporate proxy support: install host CA certificates into the container's
# trust store so tools like curl and apk can verify TLS through intercepting
# proxies (e.g., Zscaler, Fortinet, Palo Alto). No-op when not behind a
# corporate proxy. See proxy-lib.sh for details.
RUN ./tools/scripts/lib_install/setup-corporate-proxy.sh

# Point pip/pipx and curl at the system trust store. By default, Python's
# "certifi" package bundles its own CA store and ignores system certificates,
# which means pip/pipx would still fail behind a corporate proxy even after
# updating the system trust store above. These env vars override that behavior.
# Without corporate certs, these paths are the normal system bundles — harmless.
ENV SSL_CERT_FILE=/etc/ssl/certs/ca-certificates.crt \
    CURL_CA_BUNDLE=/etc/ssl/certs/ca-certificates.crt \
    REQUESTS_CA_BUNDLE=/etc/ssl/certs/ca-certificates.crt \
    PIP_CERT=/etc/ssl/certs/ca-certificates.crt

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
