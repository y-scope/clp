# Ubuntu Jammy setup

To install the dependencies required to build clp-core, follow the steps below.
These same steps are used by our Docker containers.

## Installing dependencies

:::{caution}
Before you run any commands below, you should review the scripts to ensure they will not install
any dependencies or apply any configurations that you don't expect.
:::

To install all dependencies, run the following with elevated privileges:

```shell
components/core/tools/scripts/lib_install/ubuntu-jammy/install-all.sh
```

:::{note}
If you are only interested in installing the packages built from source, ensure that you have the
desired version of [CMake] before running [install-packages-from-source.sh][src-install-script]. If
you just installed [CMake] using the pipx install [script][pipx-install-script], make sure to source
your shell rc files or open a new shell for [CMake] to become available.

These packages built from source are installed without using a packager. So if you ever need to
uninstall them, you will need to do so manually.
:::

[CMake]: https://cmake.org/
[pipx-install-script]: https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/components/core/tools/scripts/lib_install/pipx-packages/install-all.sh
[src-install-script]: https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/components/core/tools/scripts/lib_install/ubuntu-jammy/install-packages-from-source.sh
