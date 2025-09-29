from clp_py_utils.clp_config import Database
from clp_py_utils.clp_logging import get_logger
from job_orchestration.executor.compress.compression_task import compression_entry_point
from spider_py import Int8, TaskContext

# Setup logging
logger = get_logger("spider_compression_scheduler")


def convert_to_str(byte_list: list[Int8]) -> str:
    """
    Converts a list of Int8 to a UTF-8 string.
    :param byte_list: A list of Int8 representing bytes.
    :return: A UTF-8 string.
    """
    return bytes(int(byte) for byte in byte_list).decode("utf-8")


def compress(
    _: TaskContext,
    job_id: int,
    task_id: int,
    tag_ids: list[int],
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
        `job_orchestration.scheduler.constants.CompressionTaskResult`.
    """
    result_json_bytes = (
        compression_entry_point(
            job_id,
            task_id,
            tag_ids,
            convert_to_str(clp_io_config_json),
            convert_to_str(paths_to_compress_json),
            Database.model_validate_json(convert_to_str(clp_metadata_db_connection_config_json)),
            logger,
        )
        .model_dump_json()
        .encode("utf-8")
    )

    return [Int8(byte) for byte in result_json_bytes]
