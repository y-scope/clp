FROM centos:centos7.4.1708

# Install packages
RUN yum install -y \
        bzip2 \
        gcc \
        gcc-c++ \
        git \
        make \
        openssl-devel \
        openssl-static \
        python3 \
        wget \
        zlib-static

# Install packages from source
# NOTE: gcc needs to be installed first so that the remaining packages are compiled with the correct gcc
COPY install-gcc.sh /root/
RUN /root/install-gcc.sh 7.5.0

# Set LD_LIBRARY_PATH to respect the new gcc
ENV LD_LIBRARY_PATH /usr/local/lib64:/usr/local/lib

# NOTE: cmake and boost must be installed first since the remaining packages depend on them
COPY install-cmake.sh /root/
RUN /root/install-cmake.sh 3.21.2
COPY install-boost.sh /root/
RUN /root/install-boost.sh 1.76.0

COPY fmtlib.sh /root/
RUN /root/fmtlib.sh 8.0.1
COPY libarchive.sh /root/
RUN /root/libarchive.sh 3.5.1
COPY lz4.sh /root/
RUN /root/lz4.sh 1.8.2
COPY spdlog.sh /root/
RUN /root/spdlog.sh 1.9.2
COPY zstandard.sh /root/
RUN /root/zstandard.sh 1.4.9

# Set PKG_CONFIG_PATH since CentOS doesn't look in /usr/local by default
ENV PKG_CONFIG_PATH /usr/local/lib64/pkgconfig:/usr/local/lib/pkgconfig
