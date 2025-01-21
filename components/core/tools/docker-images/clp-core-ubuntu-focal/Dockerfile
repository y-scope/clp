FROM ubuntu:focal AS BASE

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

ADD ./clg ./
ADD ./clp ./
ADD ./clp-s ./
ADD ./glt ./
ADD ./make-dictionaries-readable ./

# Flatten the image
FROM scratch
COPY --from=BASE / /

WORKDIR /clp

CMD bash
