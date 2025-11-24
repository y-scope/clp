"""Make the fixtures defined in `tests/fixtures/` globally available without imports."""

pytest_plugins = [
    "tests.fixtures.integration_test_logs",
    "tests.fixtures.path_configs",
]
