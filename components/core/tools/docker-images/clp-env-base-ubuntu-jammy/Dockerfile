# syntax=docker/dockerfile:1

# Empty default for the `ca_trust` build context, so the networked RUN steps below resolve their
# mount when no host CA trust is supplied. `build.sh --with-ca-certs` overrides this stage with a
# staged directory; without it the mount is empty, the guard falls through, and the image keeps its
# own distro trust store. Nothing is ever baked in. See yscope-dev-utils exports/docker/ca-trust.
FROM scratch AS ca_trust

FROM ubuntu:jammy AS base

WORKDIR /root

RUN mkdir -p ./tools/scripts/lib_install
COPY --link ./tools/scripts/lib_install ./tools/scripts/lib_install

# Optional: override Ubuntu mirrors (e.g., organization-internal or regional).
# Usage: APT_MIRROR_URL=<url> ./build.sh
ARG APT_MIRROR_URL
RUN --mount=type=bind,from=ca_trust,target=/run/ca-trust \
    ./tools/scripts/lib_install/ca-trust-run.sh \
        ./tools/scripts/lib_install/ubuntu-jammy/configure-package-mirror.sh

# Set pipx environment vars so that the pipx scripts install into the global locations without
# passing the `--global` flag.
ENV PIPX_BIN_DIR=/usr/local/bin
ENV PIPX_HOME=/opt/pipx

RUN --mount=type=bind,from=ca_trust,target=/run/ca-trust \
    ./tools/scripts/lib_install/ca-trust-run.sh \
        ./tools/scripts/lib_install/ubuntu-jammy/install-all.sh || { \
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
ENV PIPX_BIN_DIR=/usr/local/bin
ENV PIPX_HOME=/opt/pipx
