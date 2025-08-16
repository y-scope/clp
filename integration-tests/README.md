# CLP integration tests

This Python project provides end-to-end tests for CLP using the `pytest` framework.

## Running tests

To run all integration tests:

```
task tests:integration
```

To test the core CLP binaries:

```
task tests:integration:core
```

## Testing customizations

### Using pytest markers

One can leverage `pytest` markers to selectively run tests, e.g.

```
# From the integration-tests directory
uv run python -m pytest -m clp_s
```

only runs `clp-s` related tests.

Before running tests, ensure all package script dependencies and binaries are built.

### Specifying custom test directories

You can override the default test paths by setting the following environment variables:

- **`CLP_PACKAGE_DIR`** — Path to a custom CLP package directory (where package scripts live).
- **`CLP_CORE_BINS_DIR`** — Path to CLP core binaries, for testing only the core executables.

These variables let you point the test suite at non-default build or install locations.
