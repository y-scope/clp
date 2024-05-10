import asyncio
import datetime
from enum import auto, Enum
from typing import Any, Dict, List, Optional

from job_orchestration.scheduler.constants import CompressionTaskStatus, SearchTaskStatus
from job_orchestration.scheduler.job_config import SearchConfig
from job_orchestration.scheduler.search.reducer_handler import ReducerHandlerMessageQueues
from pydantic import BaseModel, validator


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
    WAITING_FOR_REDUCER = auto()
    WAITING_FOR_DISPATCH = auto()
    RUNNING = auto()


class SearchJob(BaseModel):
    id: str
    search_config: SearchConfig
    state: InternalJobState
    start_time: Optional[datetime.datetime]
    num_archives_to_search: int
    num_archives_searched: int
    remaining_archives_for_search: List[Dict[str, Any]]
    current_sub_job_async_task_result: Optional[Any]
    reducer_acquisition_task: Optional[asyncio.Task]
    reducer_handler_msg_queues: Optional[ReducerHandlerMessageQueues]

    class Config:  # To allow asyncio.Task and asyncio.Queue
        arbitrary_types_allowed = True


class SearchTaskResult(BaseModel):
    status: SearchTaskStatus
    task_id: str
    start_time: datetime.datetime
    duration: float
