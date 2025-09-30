from __future__ import annotations

import json
from typing import Any

import spider_py
from job_orchestration.executor.compress.spider_compress import compress, convert_to_str
from job_orchestration.scheduler.scheduler.scheduler import Scheduler
from job_orchestration.scheduler.scheduler_data import CompressionTaskResult


def convert_from_str(string: str) -> list[spider_py.Int8]:
    """
    Convert a string to a list of `Int8` as utf-8 bytes.
    :param string: The string to convert.
    :return: The list of `Int8` representing the string.
    """
    return [spider_py.Int8(byte) for byte in string.encode("utf-8")]


class SpiderScheduler(Scheduler):

    def __init__(self, storage_url: str) -> None:
        self.driver = spider_py.Driver(storage_url)

    def compress(self, task_params: list[dict[str, Any]]) -> Any:
        job = spider_py.group(
            [compress for _ in range(len(task_params))],
        )
        return self.driver.submit_jobs(
            job,
            [
                spider_py.Int64(task_params["job_id"]),
                spider_py.Int64(task_params["task_id"]),
                [spider_py.Int64(tag_id) for tag_id in task_params["tag_ids"]],
                convert_from_str(task_params["clp_io_config_json"]),
                convert_from_str(task_params["paths_to_compress_json"]),
                convert_from_str(json.loads(task_params["clp_io_config_json"])),
            ],
        )

    def get_compress_result(
        self, result_handle: Any, timeout: float = 0.1
    ) -> list[CompressionTaskResult] | None:
        results = result_handle.get_results()
        if results is None:
            return None
        return [
            CompressionTaskResult.validate_model_json(convert_to_str(result)) for result in results
        ]
