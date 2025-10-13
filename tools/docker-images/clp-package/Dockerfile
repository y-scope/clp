FROM ubuntu:jammy AS base

WORKDIR /root

COPY ./tools/docker-images/clp-package/setup-scripts ./setup-scripts
RUN ./setup-scripts/install-prebuilt-packages.sh \
    && rm -rf ./setup-scripts/

# Remove cached files
RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

COPY ./build/clp-package /opt/clp
COPY --link ./build/deps/cpp/mariadb-connector-cpp-install/lib/x86_64-linux-gnu/libmariadbcpp.so \
    /opt/clp/lib/libmariadbcpp.so

# Flatten the image
FROM scratch
COPY --from=base / /

ENV CLP_HOME="/opt/clp"
ENV LD_LIBRARY_PATH="${CLP_HOME}/lib" \
    PATH="${CLP_HOME}/bin:${PATH}" \
    PYTHONPATH="${CLP_HOME}/lib/python3/site-packages"
ENV PATH="${CLP_HOME}/sbin:${PATH}"

USER 1000:1000
