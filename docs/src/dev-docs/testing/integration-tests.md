# Integration tests

The `integration-tests` directory contains a Python project that provides end-to-end tests for
CLP via the `pytest` framework.

## Running tests

To run all integration tests:

```shell
task tests:integration
```

To test the core CLP binaries:

```shell
task tests:integration:core
```

### Using `pytest` markers

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

### Specifying custom CLP binary paths

You can override the default binary paths by setting the following environment variables:

* **`CLP_CORE_BINS_DIR`**: Directory containing the CLP core binaries to test.
* **`CLP_PACKAGE_DIR`**: Directory of the CLP package to test.
