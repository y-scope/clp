To install the dependencies required to build clp-core, follow the steps below.
These same steps are used by our Docker containers.

# Installing dependencies

Before you run any commands below, you should review the scripts to ensure they
will not install any dependencies you don't expect.

* Install all dependencies:
  * ⚠ NOTE: The packages built from source (`install-packages-from-source.sh`) 
    are installed without using a packager. So if you ever need to uninstall 
    them, you will need to do so manually.

  ```bash
  ./install-all.sh
  ```

# Setup dependencies

* Enable gcc 10

  ```bash
  ln -s /opt/rh/devtoolset-10/enable /etc/profile.d/devtoolset.sh
  ```

* Set PKG_CONFIG_PATH since CentOS doesn't look in `/usr/local` by default.
  You should add this to your shell's profile/startup file (e.g., `.bashrc`).

  ```bash
  export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig:/usr/local/lib/pkgconfig
  ```

# Building CLP

* See the instructions in the [main README](../../../../README.md#build)
