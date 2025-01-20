from logging import Logger
from pathlib import Path
from typing import Optional, Tuple

from clp_py_utils.clp_config import WorkerConfig
from clp_py_utils.core import read_yaml_config_file
from clp_py_utils.s3_utils import s3_get_frozen_credentials


def load_worker_config(
    config_path: Path,
    logger: Logger,
) -> Optional[WorkerConfig]:
    """
    Loads a WorkerConfig object from the specified configuration file.
    :param config_path: Path to the configuration file.
    :param logger: Logger instance for reporting errors if loading fails.
    :return: The loaded WorkerConfig object on success, None otherwise.
    """
    try:
        return WorkerConfig.parse_obj(read_yaml_config_file(config_path))
    except Exception:
        logger.exception("Failed to load worker config")
        return None


def load_session_credentials(logger: Logger) -> Tuple[Optional[str], Optional[str]]:
    s3_frozen_credentials = s3_get_frozen_credentials()
    if s3_frozen_credentials is None:
        logger.error("Failed to get s3 credentials from local session")
        return None, None
    if s3_frozen_credentials.token is not None:
        logger.error("Not supporting session token at the moment")
        return None, None
    return s3_frozen_credentials.access_key, s3_frozen_credentials.secret_key
