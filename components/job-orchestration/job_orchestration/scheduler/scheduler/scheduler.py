from __future__ import annotations

from abc import ABC, abstractmethod
from typing import Any

from job_orchestration.scheduler.scheduler_data import CompressionTaskResult


class Scheduler(ABC):
    """Abstract base class for a scheduler framework."""

    @abstractmethod
    def compress(self, task_params: list[dict[str, Any]]) -> Any:
        """
        Starts a batch of compression tasks as a job.
        :param task_params: A list of dictionaries containing parameters for each compression task.
        :return: A handle through which to get the result of the job.
        """
        pass

    @abstractmethod
    def get_compress_result(
        self, result_handle: Any, timeout: float = 0.1
    ) -> list[CompressionTaskResult] | None:
        """
        Gets the result of a compression job.
        :param result_handle: The handle returned by the compress method.
        :param timeout: The maximum time to wait for the result. Notice that some schedulers ignore
         this parameter.
        :return: A list of task results.
        """
        pass
