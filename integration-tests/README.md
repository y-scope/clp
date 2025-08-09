# CLP Integration Tests

This Python module provides end-to-end tests for the CLP package using the `pytest` framework.
It covers package lifecycle scripts and validates compression and search functionality for
performance and correctness on large [datasets].


## Running tests
To build the package and run all integration tests:
```
task package-test
```

With the package already built, you can leverage `pytest` markers to selectively run tests, e.g.
```
# Inside the integration-tests top-level directory
uv run python -m pytest -m clp_s
```
only runs `clp-s` related tests.

To test CLP package at a custom location, override default paths with the environment variable
`CLP_PACKAGE_DIR`, and then run the `uv` command above.


[datasets]: https://docs.yscope.com/clp/main/user-guide/resources-datasets
