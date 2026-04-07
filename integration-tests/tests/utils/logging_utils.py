"""Utilities for logging during the test run."""

import datetime
import logging
import subprocess
from pathlib import Path

from tests.conftest import get_test_log_dir
from tests.utils.classes import IntegrationTestExternalAction

logger = logging.getLogger(__name__)


# TODO: `log_subprocess_output_to_file` will be phased out in favour of `log_action_output_to_file`.
def log_subprocess_output_to_file(
    proc: subprocess.CompletedProcess[str],
    cmd: list[str],
) -> None:
    """
    Logs subprocess output summary to a unique file.

    :param proc:
    :param cmd:
    """
    now = datetime.datetime.now()  # noqa: DTZ005
    test_run_id = now.strftime("%Y-%m-%d-%H-%M-%S")
    subprocess_output_file_path = (
        get_test_log_dir() / "subprocess_output" / f"{Path(cmd[0]).name}_{test_run_id}.log"
    )
    subprocess_output_file_path.parent.mkdir(parents=True, exist_ok=True)

    stdout_content = proc.stdout or "(empty)"
    stderr_content = proc.stderr or "(empty)"

    if not stdout_content.endswith("\n"):
        stdout_content += "\n"
    if not stderr_content.endswith("\n"):
        stderr_content += "\n"

    sep = "-" * 32
    lines = [
        "SUBPROCESS RUN SUMMARY\n",
        f"{sep}\n",
        f"Timestamp at completion : {now.strftime('%Y-%m-%d %H:%M:%S')}\n",
        f"Command                 : {cmd}\n",
        f"Return Code             : {proc.returncode}\n",
        "\n\n",
        "captured stdout\n",
        f"{sep}\n",
        stdout_content,
        "\n",
        "\n\n",
        "captured stderr\n",
        f"{sep}\n",
        stderr_content,
        "\n",
    ]

    with subprocess_output_file_path.open("w", encoding="utf-8") as log_file:
        log_file.writelines(lines)

    log_msg = (
        f"Subprocess returned. stdout and stderr written to log file:"
        f" '{subprocess_output_file_path}'"
    )
    logger.info(log_msg)


def format_action_failure_msg(
    reason: str, *actions: IntegrationTestExternalAction
) -> tuple[bool, str]:
    """Docstring."""
    action_log_paths: list[str] = []
    for action in actions:
        action_log_paths.append(str(action.log_file_path))
    return False, f"{reason} See relevant subprocess log(s) at: {action_log_paths}"


def log_action_output_to_file(subprocess: IntegrationTestExternalAction) -> None:
    """Docstring."""
    now = datetime.datetime.now()  # noqa: DTZ005
    test_run_id = now.strftime("%Y-%m-%d-%H-%M-%S-%f")[:-3]
    subprocess_output_file_path = (
        get_test_log_dir()
        / "subprocess_output"
        / f"{Path(subprocess.cmd[0]).name}_{test_run_id}.log"
    )
    subprocess_output_file_path.parent.mkdir(parents=True, exist_ok=True)
    subprocess.log_file_path = subprocess_output_file_path

    completed_proc = subprocess.completed_proc
    stdout_content = completed_proc.stdout or "(empty)"
    stderr_content = completed_proc.stderr or "(empty)"

    if not stdout_content.endswith("\n"):
        stdout_content += "\n"
    if not stderr_content.endswith("\n"):
        stderr_content += "\n"

    sep = "-" * 32
    lines = [
        "SUBPROCESS RUN SUMMARY\n",
        f"{sep}\n",
        f"Timestamp at completion : {now.strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]}\n",
        f"Command                 : {completed_proc.args}\n",
        f"Return Code             : {completed_proc.returncode}\n",
        "\n\n",
        "captured stdout\n",
        f"{sep}\n",
        stdout_content,
        "\n",
        "\n\n",
        "captured stderr\n",
        f"{sep}\n",
        stderr_content,
        "\n",
    ]

    with subprocess_output_file_path.open("w", encoding="utf-8") as log_file:
        log_file.writelines(lines)

    log_msg = (
        f"Subprocess returned. stdout and stderr written to log file:"
        f" '{subprocess_output_file_path}'"
    )
    logger.info(log_msg)
