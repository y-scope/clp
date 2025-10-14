FROM ubuntu:jammy AS base

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
COPY ./build/clp-package ${CLP_HOME}

ENV PATH="${CLP_HOME}/bin:${PATH}"
ENV PATH="${CLP_HOME}/sbin:${PATH}" \
    PYTHONPATH="${CLP_HOME}/lib/python3/site-packages"
    USER="clp-user"

RUN useradd --uid 1000 --shell /bin/bash --home-dir ${CLP_HOME} ${USER}
USER ${USER}
WORKDIR ${CLP_HOME}
