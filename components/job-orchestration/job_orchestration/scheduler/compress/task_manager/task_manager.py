from __future__ import annotations

from abc import ABC, abstractmethod
from typing import Any

from job_orchestration.scheduler.task_result import CompressionTaskResult


class TaskManager(ABC):
    """Abstract base class for a scheduler framework."""

    class ResultHandle(ABC):
        @abstractmethod
        def get_result(self, timeout: float = 0.1) -> list[CompressionTaskResult] | None:
            """
            Gets the result of a compression job.
            :param timeout: Maximum time (in seconds) to wait for retrieving the result. Depending
                on the implementation, this parameter may be ignored.
            :return: A list of task results.
            """

    @abstractmethod
    def submit(self, task_params: list[dict[str, Any]]) -> ResultHandle:
        """
        Submits a batch of compression tasks as a single compression job.
        :param task_params: A list of dictionaries containing parameters for each compression task.
        :return: A handle through which to get the result of the job.
        """
