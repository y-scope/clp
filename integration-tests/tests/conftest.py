"""Global pytest setup."""

import pytest

# Make the fixtures defined in `tests/fixtures/` globally available without imports.
pytest_plugins = [
    "tests.fixtures.integration_test_logs",
    "tests.fixtures.path_configs",
    "tests.fixtures.package_instance",
    "tests.fixtures.package_config",
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
    parser.addoption(
        "--job-name-contains",
        dest="JOB_NAME_CONTAINS",
        help="Filter CLP jobs by their name (case-insensitive substring match).",
    )
    parser.addoption(
        "--no-jobs",
        action="store_true",
        dest="NO_JOBS",
        help="Only validate CLP package start and stop. Do not create or run any test jobs.",
    )
