"""Global pytest setup."""

import datetime
from collections.abc import Iterator, Sequence
from pathlib import Path

import pytest

from tests.utils.utils import resolve_path_env_var

# Make the fixtures defined in `tests/fixtures/` globally available without imports.
pytest_plugins = [
    "tests.fixtures.sample_datasets",
    "tests.fixtures.path_configs",
    "tests.package_tests.fixtures",
]


BLUE = "\033[34m"
BOLD = "\033[1m"
RESET = "\033[0m"

_test_log_dir: Path | None = None


def get_test_log_dir() -> Path:
    """
    Returns the unique log directory for this test run.

    :return: The path to the log directory for this test run.
    :raise pytest.fail: If `_test_log_dir` is not initialized.
    """
    if _test_log_dir is None:
        pytest.fail("test log directory has not been initialized")
    return _test_log_dir


@pytest.hookimpl(tryfirst=True)
def pytest_configure(config: pytest.Config) -> None:  # noqa: ARG001
    """
    Initializes the unique log directory for this test run. Creates the directory under
    `$CLP_BUILD_DIR/integration_tests/test_logs/` Stores the path to `_test_log_dir` for retrieval
    via `get_test_log_dir()`.

    :param config:
    """
    global _test_log_dir  # noqa:PLW0603

    now = datetime.datetime.now()  # noqa: DTZ005
    test_run_id = now.strftime("%Y-%m-%d-%H-%M-%S")

    _test_log_dir = (
        resolve_path_env_var("CLP_BUILD_DIR")
        / "integration_tests"
        / "test_logs"
        / f"testrun_{test_run_id}"
    )
    _test_log_dir.mkdir(parents=True, exist_ok=True)


def pytest_addoption(parser: pytest.Parser) -> None:
    """
    Adds options for `pytest`.

    :param parser:
    """
    parser.addoption(
        "--base-port",
        action="store",
        default="56000",
        help="Base port for CLP package integration tests.",
    )


def pytest_itemcollected(item: pytest.Item) -> None:
    """
    Applies ANSI bold and blue formatting to each collected test's node ID, for the purposes of
    test output readability.

    :param item:
    """
    item._nodeid = f"{BOLD}{BLUE}{item.nodeid}{RESET}"  # noqa: SLF001


@pytest.hookimpl(tryfirst=True)
def pytest_report_header(config: pytest.Config) -> str:  # noqa: ARG001
    """
    Adds a line to the session header that reports the unique log directory for this test run.

    :param config:
    :return: A string containing the log directory path for display in the session header.
    """
    return f"Log directory for this test run: {get_test_log_dir()}"


def pytest_report_collectionfinish(
    config: pytest.Config,  # noqa: ARG001
    start_path: Path,  # noqa: ARG001
    items: Sequence[pytest.Item],
) -> str | list[str]:
    """
    Reports the list of collected tests for this session after collection is complete.

    :param config:
    :param start_path:
    :param items:
    :return: A formatted string listing each test name, or a warning if no tests were collected.
    """
    report: str = ""
    if len(items) == 0:
        report = f"{BOLD}No tests match the specified parameters.{RESET}\n"
    else:
        report = f"{BOLD}The following tests will run:{RESET}\n"
        for item in items:
            report += f"\t{BOLD}{BLUE}{item.nodeid}{RESET}\n"
        report += f"\n{BOLD}Running tests now...{RESET}"
    return report


@pytest.hookimpl(wrapper=True)
def pytest_runtest_setup(item: pytest.Item) -> Iterator[None]:
    """
    Redirects the pytest logging plugin's output to the test_output.log file in the unique log
    directory for this test run. The file will be created automatically if it does not already
    exist.

    :param item:
    :raise pytest.fail: If the `logging-plugin` is not registered with the plugin manager.
    """
    config = item.config
    logging_plugin = config.pluginmanager.get_plugin("logging-plugin")
    if logging_plugin is None:
        pytest.fail("Expected pytest plugin 'logging-plugin' to be registered.")

    test_output_log_file = get_test_log_dir() / "test_output.log"
    logging_plugin.set_log_path(str(test_output_log_file))
    yield
