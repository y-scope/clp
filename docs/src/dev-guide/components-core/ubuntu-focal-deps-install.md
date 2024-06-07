# Ubuntu Focal setup

To install the dependencies required to build clp-core, follow the steps below.
These same steps are used by our Docker containers.

## Installing dependencies

:::{caution}
Before you run any commands below, you should review the scripts to ensure they will not install
any dependencies or apply any configurations that you don't expect.
:::

To install all dependencies, run:

```shell
components/core/tools/scripts/lib_install/ubuntu-focal/install-all.sh
```

## Set up dependencies

Enable GCC 10 as the default compiler by running:

```shell
update-alternatives --set cc /usr/bin/gcc-10
update-alternatives --set c++ /usr/bin/g++-10
update-alternatives --set cpp /usr/bin/cpp-10
```
