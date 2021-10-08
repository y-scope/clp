FROM ubuntu:focal

env DEBIAN_FRONTEND=noninteractive

RUN apt-get update

RUN apt-get install -y ca-certificates checkinstall cmake git build-essential libboost-filesystem-dev libboost-iostreams-dev libboost-program-options-dev libmariadbclient-dev libssl-dev pkg-config wget zlib1g-dev curl python3 python3-pip

COPY lib_install/fmtlib.sh /tmp/
COPY lib_install/libarchive.sh /tmp/
COPY lib_install/lz4.sh /tmp/
COPY lib_install/spdlog.sh /tmp/
COPY lib_install/zstandard.sh /tmp/

RUN chmod +x /tmp/*
RUN /tmp/fmtlib.sh 8.0.1
RUN /tmp/libarchive.sh 3.5.1
RUN /tmp/lz4.sh 1.8.2
RUN /tmp/spdlog.sh 1.9.2
RUN /tmp/zstandard.sh 1.4.9

CMD bash