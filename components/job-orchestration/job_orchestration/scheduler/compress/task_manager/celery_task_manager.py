from __future__ import annotations

from typing import Any

import celery

from job_orchestration.executor.compress.celery_compress import compress
from job_orchestration.scheduler.compress.task_manager.task_manager import TaskManager
from job_orchestration.scheduler.task_result import CompressionTaskResult


class CeleryTaskManager(TaskManager):
    class ResultHandle(TaskManager.ResultHandle):
        def __init__(self, celery_result: celery.result.GroupResult) -> None:
            self._celery_result: celery.result.GroupResult = celery_result

        def get_result(self, timeout: float = 0.1) -> list[CompressionTaskResult] | None:
            try:
                results = self._celery_result.get(timeout=timeout)
                return [CompressionTaskResult.model_validate(res) for res in results]
            except celery.exceptions.TimeoutError:
                return None

    def submit(self, task_params: list[dict[str, Any]]) -> TaskManager.ResultHandle:
        task_instances = [compress.s(**params) for params in task_params]
        task_group = celery.group(task_instances)
        return CeleryTaskManager.ResultHandle(task_group.apply_async())
