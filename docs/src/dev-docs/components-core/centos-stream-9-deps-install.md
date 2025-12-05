# Centos Stream 9 setup

To install the dependencies required to build clp-core, follow the steps below.
These same steps are used by our Docker containers.

## Installing dependencies

:::{caution}
Before you run any commands below, you should review the scripts to ensure they will not install
any dependencies or apply any configurations that you don't expect.
:::

To install all dependencies, run the following with elevated privileges:

:::{note}
The packages built from source ([install-packages-from-source.sh][src-install-script]) are installed
without using a packager. So if you ever need to uninstall them, you will need to do so manually.
:::

```shell
components/core/tools/scripts/lib_install/centos-stream-9/install-all.sh
```

[src-install-script]: https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/components/core/tools/scripts/lib_install/centos-stream-9/install-packages-from-source.sh
