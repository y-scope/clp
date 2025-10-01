from __future__ import annotations

import itertools
import json
from typing import Any

import spider_py
from job_orchestration.executor.compress.spider_compress import compress, convert_to_str
from job_orchestration.scheduler.compress.task_manager.task_manager import TaskManager
from job_orchestration.scheduler.scheduler_result import CompressionTaskResult
from spider_py.client.job import Job


class SpiderTaskManager(TaskManager):

    class ResultHandle(TaskManager.ResultHandle):
        def __init__(self, spider_job: Job) -> None:
            self._spider_job: Job = spider_job

        def get_result(self, timeout: float = 0.1) -> list[CompressionTaskResult] | None:
            job_result = self._spider_job.get_results()
            if job_result is None:
                return None
            return [
                CompressionTaskResult.validate_model_json(convert_to_str(task_result))
                for task_result in job_result
            ]

    def __init__(self, storage_url: str) -> None:
        self._driver = spider_py.Driver(storage_url)

    def compress(self, task_params: list[dict[str, Any]]) -> TaskManager.ResultHandle:
        job = spider_py.group(
            [compress for _ in range(len(task_params))],
        )
        task_params_list = [
            [
                spider_py.Int64(task_param["job_id"]),
                spider_py.Int64(task_param["task_id"]),
                [spider_py.Int64(tag_id) for tag_id in task_param["tag_ids"]],
                _from_str(task_param["clp_io_config_json"]),
                _from_str(task_param["paths_to_compress_json"]),
                _from_str(json.loads(task_param["clp_io_config_json"])),
            ]
            for task_param in task_params
        ]
        submitted_job = self._driver.submit_jobs([job], [list(itertools.chain(*task_params_list))])
        return SpiderTaskManager.ResultHandle(submitted_job)


def _from_str(string: str) -> list[spider_py.Int8]:
    """
    Converts a string to a list of `spider_py.Int8` as utf-8 bytes.
    :param string: The string to convert.
    :return: A list of `spider_py.Int8` representing the string.
    """
    return [spider_py.Int8(byte) for byte in string.encode("utf-8")]
