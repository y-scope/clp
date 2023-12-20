FROM centos:centos7.4.1708

WORKDIR /root

RUN mkdir -p ./tools/docker-images/clp-env-base-centos7.4
ADD ./tools/docker-images/clp-env-base-centos7.4/setup-scripts ./tools/docker-images/clp-env-base-centos7.4/setup-scripts

RUN mkdir -p ./tools/scripts/lib_install
ADD ./tools/scripts/lib_install ./tools/scripts/lib_install

RUN ./tools/scripts/lib_install/centos7.4/install-all.sh

# Set PKG_CONFIG_PATH since CentOS doesn't look in /usr/local by default
ENV PKG_CONFIG_PATH /usr/local/lib64/pkgconfig:/usr/local/lib/pkgconfig

# Enable gcc 10 in login shells and non-interactive non-login shells
RUN ln -s /opt/rh/devtoolset-10/enable /etc/profile.d/devtoolset.sh

# Enable git 2.27
# NOTE: We use a script to enable the SCL git package on each git call because some Github actions
#       cannot be forced to use a bash shell that loads .bashrc
RUN cp ./tools/docker-images/clp-env-base-centos7.4/setup-scripts/git /usr/bin/git

# Load .bashrc for non-interactive bash shells
ENV BASH_ENV=/etc/bashrc

# Reset the working directory so that it's accessible by any user who runs the
# container
WORKDIR /
