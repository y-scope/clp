"""Global pytest setup."""

import pytest

# Make the fixtures defined in `tests/fixtures/` globally available without imports.
pytest_plugins = [
    "tests.fixtures.integration_test_logs",
    "tests.fixtures.path_configs",
    "tests.fixtures.package_instance",
    "tests.fixtures.package_test_config",
]


def pytest_addoption(parser: pytest.Parser) -> None:
    """
    Adds options for pytest.

    :param parser:
    """
    parser.addoption(
        "--base-port",
        action="store",
        default="55000",
        help="Base port for CLP package integration tests.",
    )
