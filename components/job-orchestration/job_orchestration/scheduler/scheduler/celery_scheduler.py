from __future__ import annotations

from typing import Any

import celery
from job_orchestration.executor.compress.celery_compress import compress
from job_orchestration.scheduler.scheduler.scheduler import Scheduler
from job_orchestration.scheduler.scheduler_data import CompressionTaskResult


class CeleryScheduler(Scheduler):

    def compress(self, task_params: list[dict[str, Any]]) -> Any:
        task_instances = [compress.s(**params) for params in task_params]
        task_group = celery.group(task_instances)
        return task_group.apply_async()

    def get_compress_result(
        self, result_handle: Any, timeout: float = 0.1
    ) -> list[CompressionTaskResult] | None:
        try:
            results = result_handle.get(timeout=timeout)
            return [CompressionTaskResult.model_validate(**res) for res in results]
        except celery.exceptions.TimeoutError:
            return None
