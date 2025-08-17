# CLP integration tests

This Python project provides end-to-end tests for CLP using the `pytest` framework.

# Contributing
Follow the steps below to develop and contribute to the project.

## Requirements

* Python >= 3.9
* [Task] >= 3.40.0
* [uv] >= 0.8

## Running tests

To run all integration tests:

```shell
task tests:integration
```

To test the core CLP binaries:

```shell
task tests:integration:core
```

## Testing customizations

### Using pytest markers

One can leverage `pytest` markers to selectively run tests, e.g.

```shell
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

## Linting

Before submitting a pull request, ensure you’ve run the linting commands below and have fixed all
violations and suppressed all warnings.

The following commands should be run from the integration-tests directory.

To run ruff format checks:
```shell
uv run ruff format --check .
```

To run ruff format fixes:
```shell
uv run ruff format .
```

To run mypy static checks:
```shell
uv run mypy .
```

To run ruff static checks:
```shell
uv run ruff --check .
```

[Task]: https://taskfile.dev
[Task]: https://taskfile.dev
[uv]: https://docs.astral.sh/uv
