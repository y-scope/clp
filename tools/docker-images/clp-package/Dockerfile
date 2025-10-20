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

ENV CLP_HOME="/opt/clp"
ENV LD_LIBRARY_PATH="${CLP_HOME}/lib" \
    PATH="${CLP_HOME}/bin:${PATH}" \
    PYTHONPATH="${CLP_HOME}/lib/python3/site-packages"
ENV PATH="${CLP_HOME}/sbin:${PATH}"

USER 1000:1000

COPY --link ./build/clp-package ${CLP_HOME}
COPY --link ./build/deps/cpp/mariadb-connector-cpp-install/lib/*/libmariadbcpp.so* ${CLP_HOME}/lib/
