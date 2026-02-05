"""Global pytest setup."""

import datetime
from collections.abc import Iterator
from pathlib import Path

import pytest

from tests.utils.logging_utils import BLUE, BOLD, RESET
from tests.utils.utils import resolve_path_env_var

# Make the fixtures defined in `tests/fixtures/` globally available without imports.
pytest_plugins = [
    "tests.fixtures.integration_test_logs",
    "tests.fixtures.path_configs",
    "tests.fixtures.package_instance",
    "tests.fixtures.package_test_config",
]


@pytest.hookimpl()
def pytest_addoption(parser: pytest.Parser) -> None:
    """
    Adds options for `pytest`.

    :param parser:
    """
    parser.addoption(
        "--base-port",
        action="store",
        default="55000",
        help="Base port for CLP package integration tests.",
    )

    # Sets up a unique log file for this test run, and stores the path to the file.
    now = datetime.datetime.now()  # noqa: DTZ005
    test_run_id = now.strftime("%Y-%m-%d-%H-%M-%S")
    log_file_path = (
        resolve_path_env_var("CLP_BUILD_DIR")
        / "integration-tests"
        / "test_logs"
        / f"testrun_{test_run_id}.log"
    )
    parser.addini(
        "log_file_path",
        help="Path to the log file for this test.",
        type="string",
        default=str(log_file_path),
    )


def pytest_itemcollected(item: pytest.Item) -> None:
    """
    Prettifies the name of the test for output purposes.

    :param item:
    """
    item._nodeid = f"{BOLD}{BLUE}{item.nodeid}{RESET}"  # noqa: SLF001


@pytest.hookimpl(tryfirst=True)
def pytest_report_header(config: pytest.Config) -> str:
    """
    Adds a field to the header at the start of the test run that reports the path to the log file
    for this test run.

    :param config:
    """
    log_file_path = Path(config.getini("log_file_path")).expanduser().resolve()
    return f"Log file path for this test run: {log_file_path}"


@pytest.hookimpl(wrapper=True)
def pytest_runtest_setup(item: pytest.Item) -> Iterator[None]:
    """
    Sets `log_file_path` as the output file for the logger.

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
