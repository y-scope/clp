"""Make the fixtures defined in `tests/fixtures/` globally available without imports."""

import pytest

pytest_plugins = [
    "tests.fixtures.integration_test_config",
    "tests.fixtures.integration_test_logs",
    "tests.fixtures.package_instance_fixtures",
    "tests.fixtures.package_config_fixtures",
]


def pytest_addoption(parser: pytest.Parser) -> None:
    """Register custom command line options."""
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
