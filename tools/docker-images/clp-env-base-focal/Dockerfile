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
        pkg-config \
        wget \
        zlib1g-dev

# Install custom packages
COPY libarchive.sh /root/
RUN /root/libarchive.sh 3.5.1
COPY lz4.sh /root/
RUN /root/lz4.sh 1.8.2
COPY spdlog.sh /root/
RUN /root/spdlog.sh 1.8.2
COPY zstandard.sh /root/
RUN /root/zstandard.sh 1.4.9
