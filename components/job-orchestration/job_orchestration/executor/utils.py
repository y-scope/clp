from typing import Optional
from logging import Logger
from clp_py_utils.clp_config import WorkerConfig
from clp_py_utils.core import read_yaml_config_file
from pathlib import Path
def try_load_worker_config(
    config_path_str: str,
    logger: Logger,
) -> Optional[WorkerConfig]:
    if config_path_str is None:
        logger.error("config_path_str can't be empty")
        return None

    try:
        return WorkerConfig.parse_obj(read_yaml_config_file(Path(config_path_str)))
    except Exception:
        logger.exception("Failed to load worker config")
        return None