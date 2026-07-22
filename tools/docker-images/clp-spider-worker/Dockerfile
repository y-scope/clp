# syntax=docker/dockerfile:1

ARG SPIDER_WORKER_IMAGE=ghcr.io/y-scope/spider/worker:main
FROM ${SPIDER_WORKER_IMAGE}

ARG SPIDER_TDL_PACKAGE_DIR=/opt/clp/lib/spider-tdl-packages
ARG CLP_HOME=/opt/clp

USER root

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends --yes \
        ca-certificates \
        libcurl4 \
        libmariadb3 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

COPY --link ./build/rust-targets/release/libclp_tdl_package.so \
    ${SPIDER_TDL_PACKAGE_DIR}/clp/libclp.so
COPY --link ./build/core/clp-s ./build/core/indexer ./build/core/log-converter ${CLP_HOME}/bin/

ENV SPIDER_TDL_PACKAGE_DIR=${SPIDER_TDL_PACKAGE_DIR} \
    CLP_HOME=${CLP_HOME}

USER spider-user
