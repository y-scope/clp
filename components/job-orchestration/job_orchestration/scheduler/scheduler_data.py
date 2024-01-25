import datetime
import typing

from pydantic import BaseModel, validator

from job_orchestration.scheduler.constants import \
    CompressionTaskStatus, \
    CompressionTaskUpdateType


class CompressionJob(BaseModel):
    id: int
    start_time: datetime.datetime
    tasks: typing.Any


class CompressionTaskUpdate(BaseModel):
    type: str
    job_id: int
    task_id: int
    status: str
    start_time: typing.Optional[datetime.datetime] = None
    duration: typing.Optional[float] = None

    @validator('type')
    def validate_type(cls, field):
        supported_types = [CompressionTaskUpdateType.COMPRESSION, CompressionTaskUpdateType.SEARCH]
        if field not in supported_types:
            raise ValueError(f"Unsupported task update type: '{field}'")
        return field

    @validator('status')
    def valid_status(cls, field):
        supported_status = [CompressionTaskStatus.SCHEDULED, CompressionTaskStatus.IN_PROGRESS,
                            CompressionTaskStatus.SUCCEEDED, CompressionTaskStatus.FAILED]
        if field not in supported_status:
            raise ValueError(f'must be one of the following {"|".join(supported_status)}')
        return field


class CompressionTaskFailureUpdate(CompressionTaskUpdate):
    error_message: str


class CompressionTaskSuccessUpdate(CompressionTaskUpdate):
    total_uncompressed_size: int
    total_compressed_size: int


class SearchJob:
    def __init__(self, async_task_result: any) -> None:
        self.async_task_result: any = async_task_result


class SearchTaskResult(BaseModel):
    success: bool
    task_id: str
