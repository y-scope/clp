# Integration tests

The `integration-tests` directory contains a Python project that provides end-to-end tests for
CLP via the `pytest` framework.

---

## Running tests

To run all integration tests:

```shell
task tests:integration
```

### Testing the CLP package

To test the CLP package:

```shell
task tests:integration:package
```

This command will build the package from your local code as well. For the complete package testing
guide, visit [Testing the CLP package](./package.md).

### Testing the CLP binaries

To test the core CLP binaries:

```shell
task tests:integration:core
```

This command will build the binaries from your local code as well. For the complete binary testing
guide, visit [Testing the CLP binaries](./binary.md).


:::{toctree}
:hidden:

package
binary
:::
