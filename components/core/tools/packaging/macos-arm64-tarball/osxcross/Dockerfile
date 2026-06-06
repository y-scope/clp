# syntax=docker/dockerfile:1

ARG OSXCROSS_BASE_IMAGE=ghcr.io/crazy-max/osxcross:14.5-r0-ubuntu
ARG OSXCROSS_RUNTIME_IMAGE=ubuntu:24.04

FROM ${OSXCROSS_BASE_IMAGE} AS osxcross-toolchain
FROM ${OSXCROSS_RUNTIME_IMAGE}

ARG LDID_REVISION=aaf8f23d7975ecdb8e77e3a8f22253e0a2352cef
ARG CLP_MACOS_ARCH=arm64
ARG MACOSX_DEPLOYMENT_TARGET
ARG OSXCROSS_MACPORTS_DEPLOYMENT_TARGET=13.0
ARG OSXCROSS_MACPORTS_PACKAGES=""
ARG MARIADB_CONNECTOR_C_VERSION=3.4.9

ENV CLP_MACOS_ARCH=${CLP_MACOS_ARCH} \
    MACOSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET} \
    PIPX_DEFAULT_PYTHON=python3 \
    UNATTENDED=1

WORKDIR /root

RUN mkdir -p ./tools/scripts
COPY tools/scripts/corporate-proxy-container.sh ./tools/scripts/corporate-proxy-container.sh
COPY tools/scripts/ca-certificates.crt ./tools/scripts/ca-certificates.crt
RUN ./tools/scripts/corporate-proxy-container.sh

ENV CURL_CA_BUNDLE=/opt/corp-ca/ca-bundle.crt \
    PIP_CERT=/opt/corp-ca/ca-bundle.crt \
    REQUESTS_CA_BUNDLE=/opt/corp-ca/ca-bundle.crt \
    SSL_CERT_FILE=/opt/corp-ca/ca-bundle.crt

RUN set -eux; \
    if command -v dnf >/dev/null 2>&1; then \
        dnf install -y \
            bash \
            bzip2 \
            clang \
            cmake \
            coreutils \
            cpio \
            curl \
            diffutils \
            file \
            findutils \
            gcc \
            gcc-c++ \
            git \
            gzip \
            jq \
            libplist-devel \
            libxml2-devel \
            llvm \
            lzma \
            make \
            openssl-devel \
            patch \
            perl \
            pkgconf-pkg-config \
            python3 \
            python3-pip \
            rsync \
            sed \
            tar \
            unzip \
            which \
            xz \
            xz-devel \
            xz-libs \
            zlib-devel; \
        dnf clean all; \
        rm -rf /var/cache/dnf /tmp/* /var/tmp/*; \
    elif command -v apt-get >/dev/null 2>&1; then \
        apt-get update; \
        DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
            bash \
            bzip2 \
            ca-certificates \
            clang \
            cmake \
            coreutils \
            cpio \
            curl \
            diffutils \
            file \
            findutils \
            g++ \
            gcc \
            git \
            gzip \
            jq \
            libplist-dev \
            libssl-dev \
            libxml2-dev \
            llvm \
            make \
            patch \
            perl \
            pkg-config \
            python3 \
            python3-pip \
            python3-venv \
            rsync \
            sed \
            tar \
            unzip \
            xz-utils \
            zlib1g-dev; \
        rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*; \
    elif command -v apk >/dev/null 2>&1; then \
        apk add --no-cache \
            bash \
            bzip2 \
            ca-certificates \
            clang \
            cmake \
            coreutils \
            cpio \
            curl \
            diffutils \
            file \
            findutils \
            g++ \
            gcc \
            git \
            gzip \
            jq \
            libplist-dev \
            libxml2-dev \
            llvm \
            make \
            openssl-dev \
            patch \
            perl \
            pkgconf \
            py3-pip \
            python3 \
            rsync \
            sed \
            tar \
            unzip \
            xz \
            xz-dev \
            zlib-dev; \
        rm -rf /tmp/* /var/tmp/*; \
    else \
        echo "ERROR: unsupported base image package manager" >&2; \
        exit 1; \
    fi

COPY --from=osxcross-toolchain /osxcross /osxcross

RUN set -eux; \
    mkdir -p /opt/osxcross; \
    target_dir=""; \
    for candidate in "${OSXCROSS_TARGET_DIR:-}" /osxcross/target /osxcross /opt/osxcross/target; do \
        if [ -n "${candidate}" ] && [ -d "${candidate}/bin" ]; then \
            target_dir="${candidate}"; \
            break; \
        fi; \
    done; \
    if [ -z "${target_dir}" ]; then \
        echo "ERROR: could not locate OSXCross target directory" >&2; \
        find / -maxdepth 3 -type d -name bin 2>/dev/null | sort | head -100 >&2; \
        exit 1; \
    fi; \
    if [ "${target_dir}" != /opt/osxcross/target ]; then \
        rm -rf /opt/osxcross/target; \
        ln -s "${target_dir}" /opt/osxcross/target; \
    fi; \
    if [ ! -f /opt/osxcross/target/toolchain.cmake ]; then \
        echo "ERROR: OSXCross toolchain.cmake not found under /opt/osxcross/target" >&2; \
        exit 1; \
    fi

ENV OSXCROSS_TARGET_DIR=/opt/osxcross/target
ENV LD_LIBRARY_PATH=/opt/osxcross/target/lib
ENV PIPX_HOME=/opt/pipx
ENV PIPX_BIN_DIR=/usr/local/bin
ENV PATH=/opt/clp-osxcross/bin:/opt/osxcross/target/bin:/usr/local/bin:${PATH}

RUN set -eux; \
    if [ -f /usr/lib64/pkgconfig/libplist.pc ] \
        && [ ! -f /usr/lib64/pkgconfig/libplist-2.0.pc ]; then \
        ln -s /usr/lib64/pkgconfig/libplist.pc /usr/lib64/pkgconfig/libplist-2.0.pc; \
    fi; \
    git clone https://github.com/ProcursusTeam/ldid.git /tmp/ldid; \
    cd /tmp/ldid; \
    git checkout "${LDID_REVISION}"; \
    make -j"$(nproc)"; \
    install -m 0755 ldid /usr/local/bin/ldid; \
    rm -rf /tmp/ldid

RUN set -eux; \
    python3 -m pip install --no-cache-dir --break-system-packages pipx \
        || python3 -m pip install --no-cache-dir pipx; \
    mkdir -p "${PIPX_HOME}" "${PIPX_BIN_DIR}"; \
    python3 -m pipx ensurepath --force

COPY components/core/tools/scripts/lib_install/pipx-packages /tmp/pipx-packages
RUN /tmp/pipx-packages/install-all.sh \
    && rm -rf /tmp/pipx-packages

RUN set -eux; \
    case "${CLP_MACOS_ARCH}" in \
        arm64) macports_packages="${OSXCROSS_MACPORTS_PACKAGES:--arm64 libarchive openssl}" ;; \
        x86_64) macports_packages="${OSXCROSS_MACPORTS_PACKAGES:-libarchive openssl}" ;; \
        *) echo "ERROR: unsupported CLP_MACOS_ARCH=${CLP_MACOS_ARCH}" >&2; exit 1 ;; \
    esac; \
    if command -v osxcross-macports >/dev/null 2>&1; then \
        for attempt in 1 2 3; do \
            if MACOSX_DEPLOYMENT_TARGET="${OSXCROSS_MACPORTS_DEPLOYMENT_TARGET}" \
                    osxcross-macports install ${macports_packages}; then \
                break; \
            fi; \
            if [ "${attempt}" = 3 ]; then \
                exit 1; \
            fi; \
            rm -rf /tmp/osxcross-macports-lock; \
            sleep "$((attempt * 5))"; \
        done; \
    else \
        echo "ERROR: osxcross-macports not found in base image" >&2; \
        exit 1; \
    fi

RUN set -eux; \
    macports_root="${OSXCROSS_TARGET_DIR}/macports/pkgs/opt/local"; \
    for openssl_root in \
        "${macports_root}/libexec/openssl3" \
        "${macports_root}/libexec/openssl11" \
        "${macports_root}/opt/openssl3" \
        "${macports_root}/opt/openssl"; do \
        if [ -d "${openssl_root}/lib/pkgconfig" ]; then \
            for pc_name in openssl libssl libcrypto; do \
                if [ -f "${openssl_root}/lib/pkgconfig/${pc_name}.pc" ]; then \
                    rm -f "${macports_root}/lib/pkgconfig/${pc_name}.pc"; \
                    cp -f "${openssl_root}/lib/pkgconfig/${pc_name}.pc" \
                        "${macports_root}/lib/pkgconfig/${pc_name}.pc"; \
                fi; \
            done; \
            break; \
        fi; \
    done

COPY components/core/tools/packaging/macos-arm64-tarball/osxcross/install-mariadb-connector-c.sh \
    /tmp/install-mariadb-connector-c.sh
RUN chmod +x /tmp/install-mariadb-connector-c.sh \
    && MARIADB_CONNECTOR_C_VERSION="${MARIADB_CONNECTOR_C_VERSION}" \
        /tmp/install-mariadb-connector-c.sh \
    && rm -f /tmp/install-mariadb-connector-c.sh

RUN set -eux; \
    rm -rf \
        "${OSXCROSS_TARGET_DIR}/macports/cache" \
        "${OSXCROSS_TARGET_DIR}/macports/tmp" \
        "${OSXCROSS_TARGET_DIR}/macports/pkgs/opt/local/man" \
        "${OSXCROSS_TARGET_DIR}/macports/pkgs/opt/local/share/doc" \
        "${OSXCROSS_TARGET_DIR}/macports/pkgs/opt/local/share/man" \
        /root/.cache \
        /tmp/* \
        /var/tmp/*

RUN mkdir -p /opt/clp-osxcross/bin
COPY components/core/tools/packaging/macos-arm64-tarball/osxcross/cmake-wrapper.sh \
    /opt/clp-osxcross/bin/cmake
RUN chmod +x /opt/clp-osxcross/bin/cmake

WORKDIR /clp
