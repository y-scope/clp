import json

from clp_py_utils.clp_logging import get_logger
from spider_py import Int64, TaskContext
from structlog.contextvars import bound_contextvars

from job_orchestration.executor.compress.compression_task import compression_entry_point

# Setup logging
logger = get_logger("spider_compression")


def compress(
    _: TaskContext,
    job_id: Int64,
    task_id: Int64,
    clp_io_config_json: bytes,
    paths_to_compress_json: bytes,
    clp_metadata_db_connection_config_json: bytes,
) -> bytes:
    """
    Compresses files using the general compression entry point.
    :param _: Spider's task context. Not used in the function.
    :param job_id:
    :param task_id:
    :param clp_io_config_json: A JSON string representation of
        `job_orchestration.scheduler.constants.ClpIoConfig`.
    :param paths_to_compress_json: A JSON string representation of
        `job_orchestration.scheduler.constants.PathToCompress`.
    :param clp_metadata_db_connection_config_json: A JSON string representation of
        `clp_py_utils.clp_config.Database`.
    :return: A JSON string representation of
        `job_orchestration.scheduler.constants.CompressionTaskResult`, encoded as a list of
        `spider_py.Int8`.
    """
    job_id_int = int(job_id)
    task_id_int = int(task_id)
    with bound_contextvars(job_id=job_id_int, task_id=task_id_int):
        result_as_json_str = json.dumps(
            compression_entry_point(
                job_id_int,
                task_id_int,
                clp_io_config_json.decode("utf-8"),
                paths_to_compress_json.decode("utf-8"),
                json.loads(clp_metadata_db_connection_config_json.decode("utf-8")),
                logger,
            )
        )

    return result_as_json_str.encode("utf-8")
