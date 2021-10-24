FROM centos:centos7.4.1708

RUN mkdir -p /root/tools/docker-images/clp-env-base-centos7.4
ADD ./tools/docker-images/clp-env-base-centos7.4/setup-scripts /root/tools/docker-images/clp-env-base-centos7.4/setup-scripts

RUN mkdir -p /root/tools/scripts/lib_install
ADD ./tools/scripts/lib_install /root/tools/scripts/lib_install

RUN cd /root && ./tools/docker-images/clp-env-base-centos7.4/setup-scripts/install-prebuilt-packages.sh
RUN cd /root && ./tools/docker-images/clp-env-base-centos7.4/setup-scripts/install-packages-from-source.sh

# Set PKG_CONFIG_PATH since CentOS doesn't look in /usr/local by default
ENV PKG_CONFIG_PATH /usr/local/lib64/pkgconfig:/usr/local/lib/pkgconfig

# Enable gcc 7
RUN echo -e "\nsource /opt/rh/devtoolset-7/enable" >> /root/.bashrc

# Enable git 2.27
RUN echo -e "\nsource /opt/rh/rh-git227/enable" >> /root/.bashrc