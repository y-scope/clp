from __future__ import annotations

import json
from typing import Any

import spider_py
from spider_py.client.job import Job

from job_orchestration.executor.compress.spider_compress import compress
from job_orchestration.scheduler.compress.task_manager.task_manager import TaskManager
from job_orchestration.scheduler.task_result import CompressionTaskResult
from job_orchestration.utils.spider_utils import int8_list_to_utf8_str, utf8_str_to_int8_list


class SpiderTaskManager(TaskManager):

    class ResultHandle(TaskManager.ResultHandle):
        def __init__(self, spider_job: Job) -> None:
            self._spider_job: Job = spider_job

        def get_result(self, timeout: float = 0.1) -> list[CompressionTaskResult] | None:
            job_results = self._spider_job.get_results()
            if job_results is None:
                return None
            if not isinstance(job_results, tuple):
                return CompressionTaskResult.model_validate_json(int8_list_to_utf8_str(job_results))
            return [
                CompressionTaskResult.model_validate_json(int8_list_to_utf8_str(task_result))
                for task_result in job_results
            ]

    def __init__(self, storage_url: str) -> None:
        self._driver = spider_py.Driver(storage_url)

    def submit(self, task_params: list[dict[str, Any]]) -> TaskManager.ResultHandle:
        job = spider_py.group(
            [compress for _ in range(len(task_params))],
        )
        job_args = []
        for task_param in task_params:
            job_args.append(spider_py.Int64(task_param["job_id"]))
            job_args.append(spider_py.Int64(task_param["task_id"]))
            if "tag_ids" in task_param and task_param["tag_ids"] is not None:
                job_args.append([spider_py.Int64(tag_id) for tag_id in task_param["tag_ids"]])
            else:
                job_args.append([])
            job_args.append(utf8_str_to_int8_list(task_param["clp_io_config_json"]))
            job_args.append(utf8_str_to_int8_list(task_param["paths_to_compress_json"]))
            job_args.append(
                utf8_str_to_int8_list(json.dumps(task_param["clp_metadata_db_connection_config"]))
            )
        submitted_job = self._driver.submit_jobs([job], [job_args])[0]
        return SpiderTaskManager.ResultHandle(submitted_job)
