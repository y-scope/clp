FROM ubuntu:jammy AS base

WORKDIR /root

COPY ./tools/docker-images/clp-package/setup-scripts ./setup-scripts
RUN ./setup-scripts/install-prebuilt-packages.sh \
    && rm -rf ./setup-scripts/

# Remove cached files
RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* \
    && apt-get update \
    && apt-get install python3-pip -y 

ENV CLP_HOME="/opt/clp"
ENV PATH="${CLP_HOME}/bin:${PATH}"
ENV PATH="${CLP_HOME}/sbin:${PATH}"
ENV PYTHONPATH="${CLP_HOME}/lib/python3/site-packages"

USER 1000:1000

COPY ./build/clp-mcp-server /opt/clp

# Flatten the image
FROM scratch
COPY --from=base / /
