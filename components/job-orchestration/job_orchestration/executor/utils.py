"""Utilities shared by job-orchestration executors."""

import logging
import os
from pathlib import Path

from clp_py_utils.clp_config import ClpConfig, WorkerConfig
from clp_py_utils.core import read_yaml_config_file


def log_file_contents(
    logger: logging.Logger,
    log_path: Path,
    level: int = logging.ERROR,
) -> None:
    """Dump the contents of a log file to the logger if it's non-empty."""
    if log_path.stat().st_size > 0:
        logger.log(level, f"Contents of {log_path.name}:\n{log_path.read_text()}")


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
    logger: logging.Logger,
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
