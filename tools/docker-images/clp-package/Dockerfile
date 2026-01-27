# syntax=docker/dockerfile:1

FROM ubuntu:jammy AS base

WORKDIR /root

COPY --link ./tools/docker-images/clp-package/setup-scripts ./setup-scripts
RUN ./setup-scripts/install-prebuilt-packages.sh \
    && rm -rf ./setup-scripts/

# Remove cached files
RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Flatten the image
FROM scratch
COPY --link --from=base / /

ARG UID=1000
ENV CLP_HOME="/opt/clp"
ENV LD_LIBRARY_PATH="${CLP_HOME}/lib" \
    PATH="${CLP_HOME}/sbin:${CLP_HOME}/bin:${PATH}" \
    PYTHONPATH="${CLP_HOME}/lib/python3/site-packages" \
    USER="clp-user"

RUN useradd --uid ${UID} --shell /bin/bash --home-dir ${CLP_HOME} ${USER}
USER ${USER}
WORKDIR ${CLP_HOME}

COPY --link --chown=${UID} ./components/package-template/src/ .
COPY --link --chown=${UID} ./build/core/clg bin/
COPY --link --chown=${UID} ./build/core/clo bin/
COPY --link --chown=${UID} ./build/core/clp bin/
COPY --link --chown=${UID} ./build/core/clp-s bin/
COPY --link --chown=${UID} ./build/core/indexer bin/
COPY --link --chown=${UID} ./build/core/log-converter bin/
COPY --link --chown=${UID} ./build/core/reducer-server bin/
COPY --link --chown=${UID} ./build/deps/cpp/mariadb-connector-cpp-install/lib/*/libmariadbcpp.so* \
    lib/
COPY --link --chown=${UID} ./build/spider/spider-build/src/spider/spider_scheduler bin/
COPY --link --chown=${UID} ./build/spider/spider-build/src/spider/spider_worker bin/
COPY --link --chown=${UID} ./build/nodejs-22/bin/node bin/node-22
COPY --link --chown=${UID} ./build/python-libs/ lib/python3/site-packages/
COPY --link --chown=${UID} ./build/rust-targets/release/api_server bin/
COPY --link --chown=${UID} ./build/rust-targets/release/log-ingestor bin/
COPY --link --chown=${UID} ./build/webui/ var/www/webui/
