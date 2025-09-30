from __future__ import annotations

import json
from typing import Any, Sequence

import spider_py
from job_orchestration.executor.compress.spider_compress import compress, convert_to_str
from job_orchestration.scheduler.scheduler.scheduler import Scheduler
from job_orchestration.scheduler.scheduler_data import CompressionTaskResult
from spider_py.client.job import Job


class SpiderScheduler(Scheduler):

    class ResultHandle(Scheduler.ResultHandle):
        def __init__(self, spider_jobs: Sequence[Job]) -> None:
            self._spider_jobs: Sequence[Job] = spider_jobs

        def get_result(self, timeout: float = 0.1) -> list[CompressionTaskResult] | None:
            results: list[CompressionTaskResult] = []
            for job in self._spider_jobs:
                job_results = job.get_results()
                if job_results is None:
                    return None
                results.extend(
                    [
                        CompressionTaskResult.validate_model_json(convert_to_str(result))
                        for result in job_results
                    ]
                )
            return results

    def __init__(self, storage_url: str) -> None:
        self._driver = spider_py.Driver(storage_url)

    def compress(self, task_params: list[dict[str, Any]]) -> Any:
        job = spider_py.group(
            [compress for _ in range(len(task_params))],
        )
        submitted_jobs = self._driver.submit_jobs(
            job,
            [
                spider_py.Int64(task_params["job_id"]),
                spider_py.Int64(task_params["task_id"]),
                [spider_py.Int64(tag_id) for tag_id in task_params["tag_ids"]],
                _from_str(task_params["clp_io_config_json"]),
                _from_str(task_params["paths_to_compress_json"]),
                _from_str(json.loads(task_params["clp_io_config_json"])),
            ],
        )
        return SpiderScheduler.ResultHandle(submitted_jobs)


def _from_str(string: str) -> list[spider_py.Int8]:
    """
    Converts a string to a list of `spider_py.Int8` as utf-8 bytes.
    :param string: The string to convert.
    :return: A list of `spider_py.Int8` representing the string.
    """
    return [spider_py.Int8(byte) for byte in string.encode("utf-8")]
