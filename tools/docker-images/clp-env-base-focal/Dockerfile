FROM ubuntu:focal

# Install packages
RUN apt update && \
    DEBIAN_FRONTEND=noninteractive apt install -y \
        ca-certificates \
        checkinstall \
        cmake \
        build-essential \
        libboost-filesystem-dev \
        libboost-iostreams-dev \
        libboost-program-options-dev \
        libssl-dev \
        wget \
        zlib1g-dev

# Install custom packages
COPY libarchive_3.5.1.sh /root/
RUN /root/libarchive_3.5.1.sh
COPY lz4_1.8.2.sh /root/
RUN /root/lz4_1.8.2.sh
COPY spdlog_1.8.2.sh /root/
RUN /root/spdlog_1.8.2.sh
COPY zstandard_1.4.9.sh /root/
RUN /root/zstandard_1.4.9.sh
