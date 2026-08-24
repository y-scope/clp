from __future__ import annotations

import logging
from typing import Any

import celery

from job_orchestration.executor.compress.celery_compress import compress
from job_orchestration.scheduler.compress.task_manager.task_manager import TaskManager
from job_orchestration.scheduler.task_result import CompressionTaskResult

logger = logging.getLogger(__name__)

TASK_RESULT_GET_INTERVAL_SECONDS = 0.005
TASK_RESULT_DEFAULT_GET_TIMEOUT_SECONDS = 10

class CeleryTaskManager(TaskManager):
    class ResultHandle(TaskManager.ResultHandle):
        def __init__(self, celery_result: celery.result.GroupResult) -> None:
            self._celery_result: celery.result.GroupResult = celery_result

        def get_result(self, timeout: float = TASK_RESULT_DEFAULT_GET_TIMEOUT_SECONDS) -> list[CompressionTaskResult] | None:
            if not self._celery_result.ready():
                return None
            try:
                results = self._celery_result.get(
                    timeout=timeout, interval=TASK_RESULT_GET_INTERVAL_SECONDS
                )
                return [CompressionTaskResult.model_validate(res) for res in results]
            except celery.exceptions.TimeoutError:
                logger.exception("Timed out waiting for task result.")
                raise
            except celery.exceptions.SoftTimeLimitExceeded:
                logger.exception("Compression task exceeded soft time limit.")
                raise

    def submit(self, task_params: list[dict[str, Any]]) -> TaskManager.ResultHandle:
        task_instances = [compress.s(**params) for params in task_params]
        task_group = celery.group(task_instances)
        return CeleryTaskManager.ResultHandle(task_group.apply_async())
