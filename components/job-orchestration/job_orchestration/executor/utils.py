from logging import Logger
from pathlib import Path
from typing import Optional

from clp_py_utils.clp_config import WorkerConfig
from clp_py_utils.core import read_yaml_config_file


def load_worker_config(
    config_path: Path,
    logger: Logger,
) -> Optional[WorkerConfig]:
    try:
        return WorkerConfig.parse_obj(read_yaml_config_file(config_path))
    except Exception:
        logger.exception("Failed to load worker config")
        return None
