"""Global pytest setup."""

import uuid
from collections.abc import Iterator
from pathlib import Path

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

    test_run_id = str(uuid.uuid4())[-4:]
    log_file_name = Path("__pytest_testrun_logs") / f"testrun_{test_run_id}.log"
    parser.addini(
        "log_file_path",
        help="Path to the log file for this test.",
        type="paths",
        default=log_file_name,
    )

@pytest.hookimpl(tryfirst=True)
def pytest_report_header(config) -> str:
    """Docstring"""
    log_file_path = Path(config.getini("log_file_path")).expanduser().resolve()
    return f"Log file path for this test run: {log_file_path}"


@pytest.hookimpl(hookwrapper=True,tryfirst=True)
def pytest_runtest_setup(item)-> Iterator[None]:
    """Docstring"""
    config = item.config
    logging_plugin = config.pluginmanager.get_plugin("logging-plugin")
    log_file_path = Path(config.getini("log_file_path"))
    logging_plugin.set_log_path(str(log_file_path))
    yield
