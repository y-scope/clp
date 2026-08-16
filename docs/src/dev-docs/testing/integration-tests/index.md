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

---

## Using `pytest` markers

To run more specific sets of tests, you can use `pytest` directly with `pytest` markers.

:::{note}
Before running tests using `pytest`, ensure that the CLP package and/or core binaries have been
built.
:::

Ensure all commands below are run from inside the `integration-tests` directory.

To list all available markers:

```shell
uv run pytest --markers
```

To run tests related to a specific marker (e.g., `clp_s`):

```shell
uv run pytest -m clp_s
```

:::{toctree}
:hidden:

package
binary
:::
