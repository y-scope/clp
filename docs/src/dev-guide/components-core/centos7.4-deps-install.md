# Centos 7.4 setup

To install the dependencies required to build clp-core, follow the steps below.
These same steps are used by our Docker containers.

## Installing dependencies

:::{caution}
Before you run any commands below, you should review the scripts to ensure they will not install
any dependencies or apply any configurations that you don't expect.
:::

To install all dependencies, run:

:::{note}
The packages built from source ([install-packages-from-source.sh][src-install-script]) are installed
without using a packager. So if you ever need to uninstall them, you will need to do so manually.
:::

```shell
components/core/tools/scripts/lib_install/centos7.4/install-all.sh
```

## Set up dependencies

* Enable gcc 10

  ```shell
  ln -s /opt/rh/devtoolset-10/enable /etc/profile.d/devtoolset.sh
  ```

* Set PKG_CONFIG_PATH since CentOS doesn't look in `/usr/local` by default.
  You should add this to your shell's profile/startup file (e.g., `.bashrc`).

  ```shell
  export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig:/usr/local/lib/pkgconfig
  ```

[src-install-script]: https://github.com/y-scope/clp/blob/main/components/core/tools/scripts/lib_install/centos7.4/install-packages-from-source.sh
