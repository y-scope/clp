import json

from clp_py_utils.clp_logging import get_logger
from job_orchestration.executor.compress.compression_task import compression_entry_point
from spider_py import TaskContext

# Setup logging
logger = get_logger("spider_compression_scheduler")


def compress(
    _: TaskContext,
    job_id: int,
    task_id: int,
    tag_ids: list[int],
    clp_io_config_json: str,
    paths_to_compress_json: str,
    clp_metadata_db_connection_config_json: str,
) -> str:
    """
    Compresses files using the general compression entry point.
    :param _: Spider's task context. Not used in the function.
    :param job_id:
    :param task_id:
    :param tag_ids:
    :param clp_io_config_json: A JSON string representation of
        `job_orchestration.scheduler.constants.ClpIoConfig`.
    :param paths_to_compress_json: A JSON string representation of
        `job_orchestration.scheduler.constants.PathToCompress`.
    :param clp_metadata_db_connection_config_json: A JSON string representation of
        `clp_py_utils.clp_config.Database`.
    :return: A JSON string representation of
        `job_orchestration.scheduler.constants.CompressionTaskResult`.
    """
    return json.load(compression_entry_point(
        job_id,
        task_id,
        tag_ids,
        clp_io_config_json,
        paths_to_compress_json,
        json.dumps(clp_metadata_db_connection_config_json),
        logger,
    ))
