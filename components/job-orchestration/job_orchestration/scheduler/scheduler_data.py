import datetime
import typing

from job_orchestration.scheduler.constants import CompressionTaskStatus, SearchJobStatus
from job_orchestration.scheduler.job_config import SearchConfig
from pydantic import BaseModel, validator


class CompressionJob(BaseModel):
    id: int
    start_time: datetime.datetime
    async_task_result: typing.Any


class CompressionTaskResult(BaseModel):
    task_id: int
    status: int
    start_time: datetime.datetime
    duration: float

    @validator("status")
    def valid_status(cls, field):
        supported_status = [CompressionTaskStatus.SUCCEEDED, CompressionTaskStatus.FAILED]
        if field not in supported_status:
            raise ValueError(f'must be one of the following {"|".join(supported_status)}')
        return field


class CompressionTaskFailureResult(CompressionTaskResult):
    error_message: str


class CompressionTaskSuccessResult(CompressionTaskResult):
    total_uncompressed_size: int
    total_compressed_size: int


class SearchSubJob(BaseModel):
    async_task_result: typing.Any


class SearchJob(BaseModel):
    id: str
    status: SearchJobStatus
    search_config: SearchConfig
    waiting_for_next_sub_job: bool
    remaining_archives_for_search: typing.List[str]
    current_sub_job: SearchSubJob


class SearchTaskResult(BaseModel):
    success: bool
    task_id: str
