import datetime
import json
import typing
from typing import Dict

import msgpack
import zstandard
from celery.result import AsyncResult
from pydantic import BaseModel, validator


class TaskUpdate(BaseModel):
    job_id: int
    task_id: int
    status: str

    @validator('status')
    def valid_status(cls, field):
        supported_status = ['COMPRESSING', 'COMPLETED', 'FAILED']
        if field not in supported_status:
            raise ValueError(f'must be one of the following {"|".join(supported_status)}')
        return field


class TaskCompletionUpdate(TaskUpdate):
    total_uncompressed_size: int
    total_compressed_size: int


class TaskFailureUpdate(TaskUpdate):
    error_message: str


class Task(BaseModel):
    task_id: int
    task_status: str
    priority: int = 1
    clp_paths_to_compress: bytes
    task_start_time: datetime.datetime = None
    instance: AsyncResult = None

    class Config:
        arbitrary_types_allowed = True

    def get_clp_paths_to_compress_json(self, dctx: zstandard.ZstdDecompressor = None):
        if dctx is None:
            dctx = zstandard.ZstdDecompressor()
        return json.dumps(msgpack.unpackb(dctx.decompress(self.clp_paths_to_compress)))


class Job(BaseModel):
    job_id: int
    job_status: str
    job_start_time: datetime.datetime
    clp_config: bytes
    num_tasks: typing.Optional[int]
    num_tasks_completed: int
    tasks: Dict[int, Task] = {}

    def get_clp_config_json(self, dctx: zstandard.ZstdDecompressor = None):
        if not dctx:
            dctx = zstandard.ZstdDecompressor()
        return json.dumps(msgpack.unpackb(dctx.decompress(self.clp_config)))
