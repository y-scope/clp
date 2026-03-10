# syntax=docker/dockerfile:1

FROM dokken/centos-stream-9 AS base

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
COPY --link ./tools/scripts/lib_install ./tools/scripts/lib_install
# Corporate proxy support: install host CA certificates into the container's
# trust store so tools like curl and dnf can verify TLS through intercepting
# proxies (e.g., Zscaler, Fortinet, Palo Alto). No-op when not behind a
# corporate proxy. See proxy-lib.sh for details.
RUN ./tools/scripts/lib_install/setup-corporate-proxy.sh

# Point pip/pipx and curl at the system trust store. By default, Python's
# "certifi" package bundles its own CA store and ignores system certificates,
# which means pip/pipx would still fail behind a corporate proxy even after
# updating the system trust store above. These env vars override that behavior.
# Without corporate certs, these paths are the normal system bundles — harmless.
ENV SSL_CERT_FILE=/etc/pki/tls/certs/ca-bundle.crt \
    CURL_CA_BUNDLE=/etc/pki/tls/certs/ca-bundle.crt \
    REQUESTS_CA_BUNDLE=/etc/pki/tls/certs/ca-bundle.crt \
    PIP_CERT=/etc/pki/tls/certs/ca-bundle.crt

# Optional: override CentOS Stream mirrors (e.g., organization-internal or regional).
# Usage: DNF_MIRROR_BASE_URL=<url> ./build.sh
# CentOS Stream 9 repos use metalink= (not mirrorlist=) and have no commented baseurl,
# so we comment out metalink and add baseurl lines for each enabled repo section.
ARG DNF_MIRROR_BASE_URL
RUN if [ -n "${DNF_MIRROR_BASE_URL:-}" ]; then \
        sed -i 's|^metalink=|#metalink=|g' /etc/yum.repos.d/centos.repo && \
        sed -i '/^\[baseos\]$/a baseurl='"${DNF_MIRROR_BASE_URL}"'/9-stream/BaseOS/$basearch/os/' /etc/yum.repos.d/centos.repo && \
        sed -i '/^\[appstream\]$/a baseurl='"${DNF_MIRROR_BASE_URL}"'/9-stream/AppStream/$basearch/os/' /etc/yum.repos.d/centos.repo && \
        sed -i '/^\[crb\]$/a baseurl='"${DNF_MIRROR_BASE_URL}"'/9-stream/CRB/$basearch/os/' /etc/yum.repos.d/centos.repo && \
        dnf clean all && dnf makecache; \
    fi

# Set pipx environment vars so that the pipx scripts install into the global locations without
# passing the `--global` flag.
ENV PIPX_BIN_DIR=/usr/local/bin
ENV PIPX_HOME=/opt/pipx

RUN ./tools/scripts/lib_install/centos-stream-9/install-all.sh || { \
        echo ""; \
        echo "==============================================================="; \
        echo "ERROR: Package installation failed."; \
        echo ""; \
        echo "If the error above mentions download failures, timeouts, or"; \
        echo "unreachable mirrors, try overriding with a different mirror:"; \
        echo "  DNF_MIRROR_BASE_URL=<url> ./build.sh"; \
        echo ""; \
        echo "Use your organization's internal mirror or pick one from:"; \
        echo "  https://www.centos.org/download/mirrors/"; \
        echo "==============================================================="; \
        exit 1; \
    }

# Remove cached files
RUN dnf clean all \
    && rm -rf /tmp/* /var/tmp/*

# Flatten the image
FROM scratch
COPY --link --from=base / /
