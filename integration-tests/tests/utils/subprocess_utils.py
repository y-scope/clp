"""Module docstring."""

import logging
import subprocess
from pathlib import Path

import pytest

from tests.utils.logging_utils import log_subprocess_output_to_file

DEFAULT_CMD_TIMEOUT_SECONDS = 120.0
logger = logging.getLogger(__name__)


def run_and_log_subprocess(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    """Docstring."""
    log_msg = f"Running '{Path(cmd[0]).name}' subprocess. Command: {cmd}"
    logger.info(log_msg)
    proc = subprocess.run(
        cmd,
        capture_output=True,
        timeout=DEFAULT_CMD_TIMEOUT_SECONDS,
        check=False,
        text=True,
    )

    log_subprocess_output_to_file(proc, cmd)

    if proc.returncode != 0:
        pytest.fail(f"Subprocess '{Path(cmd[0]).name}' returned a non-zero exit code.")

    return proc
