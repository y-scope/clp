"""Utilities shared by job-orchestration executors."""

import os
from logging import Logger
from pathlib import Path

from clp_py_utils.clp_config import ClpConfig, WorkerConfig
from clp_py_utils.core import read_yaml_config_file


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
