import datetime
import json
import typing
from typing import Dict

import msgpack
import zstandard
from celery.result import AsyncResult
from pydantic import BaseModel, validator

from .constants import \
    TASK_QUEUE_LOWEST_PRIORITY, \
    TASK_QUEUE_HIGHEST_PRIORITY, \
    TaskStatus, \
    TaskUpdateType


class TaskUpdate(BaseModel):
    type: str
    job_id: int
    task_id: int
    status: str
    start_time: typing.Optional[datetime.datetime] = None
    duration: typing.Optional[float] = None

    @validator('type')
    def validate_type(cls, field):
        supported_types = [TaskUpdateType.COMPRESSION, TaskUpdateType.SEARCH]
        if field not in supported_types:
            raise ValueError(f"Unsupported task update type: '{field}'")
        return field

    @validator('status')
    def valid_status(cls, field):
        supported_status = [TaskStatus.SCHEDULED, TaskStatus.IN_PROGRESS,
                            TaskStatus.SUCCEEDED, TaskStatus.FAILED]
        if field not in supported_status:
            raise ValueError(f'must be one of the following {"|".join(supported_status)}')
        return field


class TaskFailureUpdate(TaskUpdate):
    error_message: str


class CompressionTaskSuccessUpdate(TaskUpdate):
    total_uncompressed_size: int
    total_compressed_size: int


class SearchTask(BaseModel):
    id: int
    status: str
    priority: int = TASK_QUEUE_HIGHEST_PRIORITY
    start_time: typing.Optional[datetime.datetime] = None
    job_id: int
    archive_id: str
    cancelled: bool = False
    instance: AsyncResult = None

    # This is necessary so we can store an AsyncResult in the task even though
    # pydantic has no validator for it
    class Config:
        arbitrary_types_allowed = True


class CompressionJob(BaseModel):
    id: int
    start_time: datetime.datetime
    tasks: typing.Any


class SearchJob(BaseModel):
    id: int
    search_config: bytes
    status: str
    start_time: datetime.datetime
    num_tasks: int
    num_tasks_completed: int = 0
    tasks: Dict[int, SearchTask] = {}

    def get_search_config_json_str(self, dctx: zstandard.ZstdDecompressor = None):
        if not dctx:
            dctx = zstandard.ZstdDecompressor()
        return json.dumps(msgpack.unpackb(dctx.decompress(self.search_config)))
