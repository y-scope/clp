import asyncio
import datetime
from abc import ABC, abstractmethod
from enum import auto, Enum
from typing import Any, Dict, List, Optional

from job_orchestration.scheduler.constants import (
    CompressionTaskStatus,
    QueryJobType,
    QueryTaskStatus,
)
from job_orchestration.scheduler.job_config import ExtractConfig, QueryConfig, SearchConfig
from job_orchestration.scheduler.query.reducer_handler import ReducerHandlerMessageQueues
from pydantic import BaseModel, Field, validator


class CompressionJob(BaseModel):
    id: int
    start_time: datetime.datetime
    async_task_result: Any


class CompressionTaskResult(BaseModel):
    task_id: int
    status: int
    duration: float
    error_message: Optional[str]

    @validator("status")
    def valid_status(cls, field):
        supported_status = [CompressionTaskStatus.SUCCEEDED, CompressionTaskStatus.FAILED]
        if field not in supported_status:
            raise ValueError(f'must be one of the following {"|".join(supported_status)}')
        return field


class InternalJobState(Enum):
    PENDING = auto()
    WAITING_FOR_REDUCER = auto()
    WAITING_FOR_DISPATCH = auto()
    RUNNING = auto()


class QueryJob(BaseModel, ABC):
    id: str
    state: InternalJobState
    start_time: Optional[datetime.datetime]
    current_sub_job_async_task_result: Optional[Any]

    @abstractmethod
    def type(self) -> QueryJobType: ...

    @abstractmethod
    def job_config(self) -> QueryConfig: ...


class ExtractJob(QueryJob):
    extract_config: ExtractConfig
    archive_id: str

    def type(self) -> QueryJobType:
        return QueryJobType.EXTRACT_IR

    def job_config(self) -> QueryConfig:
        return self.extract_config


class SearchJob(QueryJob):
    search_config: SearchConfig
    num_archives_to_search: int
    num_archives_searched: int
    remaining_archives_for_search: List[Dict[str, Any]]
    reducer_acquisition_task: Optional[asyncio.Task]
    reducer_handler_msg_queues: Optional[ReducerHandlerMessageQueues]

    def type(self) -> QueryJobType:
        return QueryJobType.SEARCH

    def job_config(self) -> QueryConfig:
        return self.search_config

    class Config:  # To allow asyncio.Task and asyncio.Queue
        arbitrary_types_allowed = True


class QueryTaskResult(BaseModel):
    status: QueryTaskStatus
    task_id: str
    duration: float
    error_log_path: Optional[str]
