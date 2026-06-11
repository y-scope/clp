"""Utilities shared by job-orchestration executors."""

import logging
import os
from logging import Logger
from pathlib import Path

from celery.app.defaults import DEFAULT_PROCESS_LOG_FMT
from clp_py_utils.clp_config import ClpConfig, WorkerConfig
from clp_py_utils.core import read_yaml_config_file


def add_container_log_handler(logger: logging.Logger) -> None:
    """
    Add a StreamHandler that writes to the container's real stderr (fd 2).

    In Celery workers, ``-f`` redirects ``sys.stdout`` and ``sys.stderr`` to
    the log file, so regular ``StreamHandler(sys.stderr)`` would write to the
    file instead of the container's stderr.  This handler writes directly to
    file descriptor 2, ensuring output reaches ``docker compose logs`` /
    ``kubectl logs``.
    """
    # dup(2) so the handler owns its own fd and doesn't close the original
    stderr_stream = os.fdopen(os.dup(2), "w")
    handler = logging.StreamHandler(stderr_stream)
    handler.setFormatter(logging.Formatter(DEFAULT_PROCESS_LOG_FMT))
    logger.addHandler(handler)


def load_clp_config_from_config_path_env_var() -> ClpConfig:
    """
    Loads the CLP config from `CLP_CONFIG_PATH`.

    :return: The loaded `ClpConfig` object.
    """
    config_path = os.getenv("CLP_CONFIG_PATH")
    if config_path is None or "" == config_path:
        return ClpConfig()
    config = read_yaml_config_file(Path(config_path))
    if config is None:
        return ClpConfig()
    return ClpConfig.model_validate(config)


def load_worker_config(
    config_path: Path,
    logger: Logger,
) -> WorkerConfig | None:
    """
    Loads a WorkerConfig object from the specified configuration file.
    :param config_path: Path to the configuration file.
    :param logger: Logger instance for reporting errors if loading fails.
    :return: The loaded WorkerConfig object on success, None otherwise.
    """
    try:
        return WorkerConfig.model_validate(read_yaml_config_file(config_path))
    except Exception:
        logger.exception("Failed to load worker config")
        return None
