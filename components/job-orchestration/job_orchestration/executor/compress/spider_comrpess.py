from clp_py_utils.clp_logging import get_logger
from job_orchestration.executor.compress.compression_task import general_compress
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
    clp_metadata_db_connection_config: dict[str, bool | int | str | None],
) -> dict[str, int | float | str | None]:
    """
    Compress files using the general compression function.
    :param _:
    :param job_id:
    :param task_id:
    :param tag_ids:
    :param clp_io_config_json: A Json string representing `ClpIoConfig`.
    :param paths_to_compress_json: A Json string representing `PathsToCompress`.
    :param clp_metadata_db_connection_config: A dict representing `Database`.
    :return: A dict representing `CompressionTaskResult`.
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
