import json

from clp_py_utils.clp_logging import get_logger
from job_orchestration.executor.compress.compression_task import compression_entry_point
from job_orchestration.utils.spider_utils import int8_list_to_utf8_str, utf8_str_to_int8_list
from spider_py import Int8, Int64, TaskContext

# Setup logging
logger = get_logger("spider_compression")


def compress(
    _: TaskContext,
    job_id: Int64,
    task_id: Int64,
    tag_ids: list[Int64],
    clp_io_config_json: list[Int8],
    paths_to_compress_json: list[Int8],
    clp_metadata_db_connection_config_json: list[Int8],
) -> list[Int8]:
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
        `job_orchestration.scheduler.constants.CompressionTaskResult`, encoded as a list of
        `spider_py.Int8`.
    """
    result_as_json_str = json.dumps(
        compression_entry_point(
            int(job_id),
            int(task_id),
            [int(tag_id) for tag_id in tag_ids],
            int8_list_to_utf8_str(clp_io_config_json),
            int8_list_to_utf8_str(paths_to_compress_json),
            json.loads(int8_list_to_utf8_str(clp_metadata_db_connection_config_json)),
            logger,
        )
    )

    return utf8_str_to_int8_list(result_as_json_str)
