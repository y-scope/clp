"""Global pytest setup."""

import pytest

# Make the fixtures defined in `tests/fixtures/` globally available without imports.
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
    parser.addini(
        "BASE_PORT",
        "Base port for CLP package integration tests.",
        default="55000",
    )
    parser.addoption(
        "--job-name",
        dest="JOB_NAME",
        help="Filter CLP jobs by substring of their job_name.",
    )
    parser.addoption(
        "--no-jobs",
        action="store_true",
        dest="NO_JOBS",
        help=("Only validate CLP package start and stop. Do not create or run any test jobs."),
    )
