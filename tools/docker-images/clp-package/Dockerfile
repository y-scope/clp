FROM ubuntu:jammy AS base

COPY ./build/clp-package /opt/clp

WORKDIR /root

COPY ./tools/docker-images/clp-package/setup-scripts ./setup-scripts
RUN ./setup-scripts/install-prebuilt-packages.sh \
    && rm -rf ./setup-scripts/

# Remove cached files
RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Flatten the image
FROM scratch
COPY --from=base / /

ENV CLP_HOME="/opt/clp"
ENV PATH="${CLP_HOME}/bin:${PATH}"
ENV PATH="${CLP_HOME}/sbin:${PATH}"
ENV PYTHONPATH="${CLP_HOME}/lib/python3/site-packages"

USER 1000:1000
