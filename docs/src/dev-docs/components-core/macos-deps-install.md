# macOS setup

To install the dependencies required to build clp-core, follow the steps below.
These same steps are used by our [GitHub workflow][gh-workflow].

## Installing dependencies

:::{caution}
Before you run any commands below, you should review the scripts to ensure they
will not install any dependencies you don't expect.
:::

To install all dependencies, run:

```shell
components/core/tools/scripts/lib_install/macos/install-all.sh
```

[gh-workflow]: https://github.com/y-scope/clp/blob/DOCS_VAR_CLP_GIT_REF/.github/workflows/clp-core-build-macos.yaml
