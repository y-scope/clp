FROM ubuntu:jammy AS base

COPY ./build/clp-package /opt/clp

WORKDIR /root

COPY ./tools/docker-images/clp-package-ubuntu-jammy/setup-scripts ./setup-scripts
RUN ./setup-scripts/install-prebuilt-packages.sh \
    && rm -rf ./setup-scripts/

# Remove cached files
RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Flatten the image
FROM scratch
COPY --from=base / /

ENV PATH="/opt/clp/sbin:${PATH}"
ENV PATH="/opt/clp/bin:${PATH}"
ENV PYTHONPATH="/opt/clp/lib/python3/site-packages"
ENV CLP_HOME="/opt/clp"

USER 1000:1000
