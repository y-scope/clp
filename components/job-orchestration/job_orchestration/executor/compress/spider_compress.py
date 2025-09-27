from clp_py_utils.clp_logging import get_logger
from job_orchestration.executor.compress.compression_task import general_compress
from spider_py import TaskContext

# Setup logging
logger = get_logger("spider_compression_scheduler")


def compress(
    _: TaskContext,
    job_id: int,
    task_id: int,
    tag_ids: list[int] | None,
    clp_io_config_json: str,
    paths_to_compress_json: str,
    clp_metadata_db_connection_config: dict[str, bool | int | str | None],
) -> dict[str, int | float | str | None]:
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
    :param clp_metadata_db_connection_config: A dict representation of
        `clp_py_utils.clp_config.Database`.
    :return: A dict representation of `job_orchestration.scheduler.constants.CompressionTaskResult`.
    """
    return general_compress(
        job_id,
        task_id,
        tag_ids,
        clp_io_config_json,
        paths_to_compress_json,
        clp_metadata_db_connection_config,
        logger,
    )
