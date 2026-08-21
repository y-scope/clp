# syntax=docker/dockerfile:1

# Empty default for the `ca_trust` build context, so the networked RUN steps below resolve their
# mount when no host CA trust is supplied. `build.sh --with-ca-certs` overrides this stage with a
# staged directory; without it the mount is empty, the guard falls through, and the image keeps its
# own distro trust store. Nothing is ever baked in. See yscope-dev-utils exports/docker/ca-trust.
FROM scratch AS ca_trust

# Multi-arch base image (amd64, arm64, ...). Build with `--platform` to select the
# target architecture; see build.sh (PLATFORM env var for local cross-builds).
FROM quay.io/pypa/musllinux_1_2

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
COPY --link ./tools/scripts/lib_install ./tools/scripts/lib_install

# Optional: override Alpine mirrors (e.g., organization-internal or regional).
# Usage: APK_MIRROR_URL=<url> ./build.sh
ARG APK_MIRROR_URL
RUN --mount=type=bind,from=ca_trust,target=/run/ca-trust \
    ./tools/scripts/lib_install/ca-trust-run.sh \
        ./tools/scripts/lib_install/musllinux_1_2/configure-package-mirror.sh

RUN pipx uninstall cmake
RUN pipx uninstall uv

RUN --mount=type=bind,from=ca_trust,target=/run/ca-trust \
    ./tools/scripts/lib_install/ca-trust-run.sh \
        ./tools/scripts/lib_install/musllinux_1_2/install-all.sh || { \
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
