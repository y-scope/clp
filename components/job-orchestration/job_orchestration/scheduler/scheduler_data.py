import datetime
import typing

from job_orchestration.scheduler.constants import CompressionTaskStatus
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


class SearchJob:
    def __init__(self, async_task_result: any) -> None:
        self.async_task_result: any = async_task_result


class SearchTaskResult(BaseModel):
    success: bool
    task_id: str
