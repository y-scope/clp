# syntax=docker/dockerfile:1

# Empty default for the `ca_trust` build context, so the networked RUN steps below resolve their
# mount when no host CA trust is supplied. `build.sh --with-ca-certs` overrides this stage with a
# staged directory; without it the mount is empty, the guard falls through, and the image keeps its
# own distro trust store. Nothing is ever baked in. See yscope-dev-utils exports/docker/ca-trust.
FROM scratch AS ca_trust

# Multi-arch base image (amd64, arm64, ...). Build with `--platform` to select the
# target architecture; see build.sh (PLATFORM env var for local cross-builds).
FROM quay.io/pypa/manylinux_2_28

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
COPY --link ./tools/scripts/lib_install ./tools/scripts/lib_install

# Optional: override AlmaLinux mirrors (e.g., organization-internal or regional).
# Usage: DNF_MIRROR_BASE_URL=<url> ./build.sh
ARG DNF_MIRROR_BASE_URL
RUN --mount=type=bind,from=ca_trust,target=/run/ca-trust \
    ./tools/scripts/lib_install/ca-trust-run.sh \
        ./tools/scripts/lib_install/manylinux_2_28/configure-package-mirror.sh

RUN pipx uninstall cmake
RUN pipx uninstall uv

RUN --mount=type=bind,from=ca_trust,target=/run/ca-trust \
    ./tools/scripts/lib_install/ca-trust-run.sh \
        ./tools/scripts/lib_install/manylinux_2_28/install-all.sh || { \
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
