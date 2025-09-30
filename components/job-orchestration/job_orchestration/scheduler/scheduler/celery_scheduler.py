from __future__ import annotations

from typing import Any

import celery
from job_orchestration.executor.compress.celery_compress import compress
from job_orchestration.scheduler.scheduler.scheduler import Scheduler
from job_orchestration.scheduler.scheduler_data import CompressionTaskResult


class CeleryScheduler(Scheduler):

    class ResultHandle(Scheduler.ResultHandle):
        def __init__(self, celery_result: celery.result.GroupResult) -> None:
            self._celery_result: celery.result.GroupResult = celery_result

        def get_result(self, timeout: float = 0.1) -> list[CompressionTaskResult] | None:
            try:
                results = self._celery_result.get(timeout=timeout)
                return [CompressionTaskResult.model_validate(**res) for res in results]
            except celery.exceptions.TimeoutError:
                return None

    def compress(self, task_params: list[dict[str, Any]]) -> Scheduler.ResultHandle:
        task_instances = [compress.s(**params) for params in task_params]
        task_group = celery.group(task_instances)
        return CeleryScheduler.ResultHandle(task_group)
