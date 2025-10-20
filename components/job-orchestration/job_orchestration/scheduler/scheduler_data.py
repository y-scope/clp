import asyncio
import datetime
from abc import ABC, abstractmethod
from enum import auto, Enum
from typing import Any, Dict, List, Optional

from pydantic import BaseModel, ConfigDict

from job_orchestration.scheduler.compress.task_manager.task_manager import TaskManager
from job_orchestration.scheduler.constants import (
    QueryJobType,
    QueryTaskStatus,
)
from job_orchestration.scheduler.job_config import (
    ExtractIrJobConfig,
    ExtractJsonJobConfig,
    QueryJobConfig,
    SearchJobConfig,
)
from job_orchestration.scheduler.query.reducer_handler import ReducerHandlerMessageQueues


class CompressionJob(BaseModel):
    # Allow the use of `TaskManager.ResultHandle`
    model_config = ConfigDict(arbitrary_types_allowed=True)

    id: int
    start_time: datetime.datetime
    result_handle: TaskManager.ResultHandle


class InternalJobState(Enum):
    WAITING_FOR_REDUCER = auto()
    WAITING_FOR_DISPATCH = auto()
    RUNNING = auto()


class QueryJob(BaseModel, ABC):
    id: str
    state: InternalJobState
    start_time: Optional[datetime.datetime] = None
    current_sub_job_async_task_result: Optional[Any] = None

    @abstractmethod
    def get_type(self) -> QueryJobType: ...

    @abstractmethod
    def get_config(self) -> QueryJobConfig: ...


class ExtractIrJob(QueryJob):
    extract_ir_config: ExtractIrJobConfig

    def get_type(self) -> QueryJobType:
        return QueryJobType.EXTRACT_IR

    def get_config(self) -> QueryJobConfig:
        return self.extract_ir_config


class ExtractJsonJob(QueryJob):
    extract_json_config: ExtractJsonJobConfig

    def get_type(self) -> QueryJobType:
        return QueryJobType.EXTRACT_JSON

    def get_config(self) -> QueryJobConfig:
        return self.extract_json_config


class SearchJob(QueryJob):
    # To allow asyncio.Task and asyncio.Queue
    model_config = ConfigDict(arbitrary_types_allowed=True)

    search_config: SearchJobConfig
    num_archives_to_search: int
    num_archives_searched: int
    remaining_archives_for_search: List[Dict[str, Any]]
    reducer_acquisition_task: Optional[asyncio.Task] = None
    reducer_handler_msg_queues: Optional[ReducerHandlerMessageQueues] = None

    def get_type(self) -> QueryJobType:
        return QueryJobType.SEARCH_OR_AGGREGATION

    def get_config(self) -> QueryJobConfig:
        return self.search_config


class QueryTaskResult(BaseModel):
    status: QueryTaskStatus
    task_id: int
    duration: float
    error_log_path: Optional[str] = None
