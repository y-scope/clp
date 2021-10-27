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
RUN ln -s /opt/rh/devtoolset-7/enable /etc/profile.d/devtoolset.sh

# Enable git 2.27
RUN ln -s /opt/rh/rh-git227/enable /etc/profile.d/git.sh

# Load .bashrc for non-interactive bash shells
ENV BASH_ENV=/root/.bashrc
