FROM ubuntu:jammy AS base

WORKDIR /root

COPY --link ./tools/docker-images/clp-package/setup-scripts ./setup-scripts
RUN ./setup-scripts/install-prebuilt-packages.sh \
    && rm -rf ./setup-scripts/

# Remove cached files
RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

ENV CLP_HOME="/opt/clp"
ENV PATH="${CLP_HOME}/bin:${PATH}"
ENV PATH="${CLP_HOME}/sbin:${PATH}"
ENV PYTHONPATH="${CLP_HOME}/lib/python3/site-packages"

USER 1000:1000

COPY --link ./build/clp-package /opt/clp

# Flatten the image
FROM scratch
COPY --link --from=base / /
