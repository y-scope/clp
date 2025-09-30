from typing import Optional

from job_orchestration.scheduler.constants import CompressionTaskStatus
from pydantic import BaseModel, validator


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
