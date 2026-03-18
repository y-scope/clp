"""Utilities for logging during the test run."""

import datetime
import logging
import subprocess
from pathlib import Path

from tests.conftest import get_test_log_dir

logger = logging.getLogger(__name__)


def log_subprocess_output_to_file(
    subprocess: subprocess.CompletedProcess[str],
    cmd: list[str],
) -> None:
    """Docstring."""
    now = datetime.datetime.now()  # noqa: DTZ005
    test_run_id = now.strftime("%Y-%m-%d-%H-%M-%S")
    subprocess_output_file_path = (
        get_test_log_dir() / "subprocess_output" / f"{Path(cmd[0]).name}_{test_run_id}.log"
    )
    subprocess_output_file_path.parent.mkdir(parents=True, exist_ok=True)

    stdout_content = subprocess.stdout or "(empty)"
    stderr_content = subprocess.stderr or "(empty)"

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
        f"Return Code             : {subprocess.returncode}\n",
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
