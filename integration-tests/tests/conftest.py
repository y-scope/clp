"""Global pytest setup."""

import uuid
from collections.abc import Iterator
from pathlib import Path

import pytest

from tests.utils.logging_utils import BLUE, BOLD, RESET

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

    test_run_id = str(uuid.uuid4())[-4:]
    log_file_name = Path("__pytest_logs") / f"testrun_{test_run_id}.log"
    parser.addini(
        "log_file_path",
        help="Path to the log file for this test.",
        type="paths",
        default=log_file_name,
    )


def pytest_itemcollected(item: pytest.Item) -> None:
    """Prettify the name of the test for output purposes."""
    item._nodeid = f"{BOLD}{BLUE}Running test: {item.name}{RESET}"  # noqa: SLF001


@pytest.hookimpl(tryfirst=True)
def pytest_report_header(config: pytest.Config) -> str:
    """
    Adds a field to the header at the start of the test run.

    :param config:
    """
    log_file_path = Path(config.getini("log_file_path")).expanduser().resolve()
    return f"Log file path for this test run: {log_file_path}"


@pytest.hookimpl(hookwrapper=True, tryfirst=True)
def pytest_runtest_setup(item: pytest.Item) -> Iterator[None]:
    """
    Sets the output file for the logger to the log file path for this test run.

    :param item:
    """
    config = item.config
    logging_plugin = config.pluginmanager.get_plugin("logging-plugin")
    if logging_plugin is None:
        err_msg = "Expected pytest plugin 'logging-plugin' to be registered."
        raise RuntimeError(err_msg)

    log_file_path = Path(config.getini("log_file_path"))
    logging_plugin.set_log_path(str(log_file_path))
    yield
