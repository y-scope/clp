FROM ubuntu:noble AS base

# Install runtime dependencies
# TODO: Investigate why libssl-dev is a hidden dependency of clp-s
RUN apt-get update \
    && apt-get install -y \
    libcurl4 \
    libmariadb-dev \
    libssl-dev

# Remove cached files
RUN apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

WORKDIR /clp

COPY ./clg ./
COPY ./clp ./
COPY ./clp-s ./
COPY ./glt ./
COPY ./make-dictionaries-readable ./

# Flatten the image
FROM scratch
COPY --from=base / /

WORKDIR /clp

CMD ["bash"]
