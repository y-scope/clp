from pydantic import BaseModel, field_validator

from job_orchestration.scheduler.constants import CompressionTaskStatus


class CompressionTaskResult(BaseModel):
    task_id: int
    status: int
    duration: float
    error_message: str | None = None

    @field_validator("status")
    def valid_status(cls, value):
        if value != CompressionTaskStatus.SUCCEEDED and value != CompressionTaskStatus.FAILED:
            raise ValueError(
                f"Must be either {CompressionTaskStatus.SUCCEEDED} or"
                f" {CompressionTaskStatus.FAILED}"
            )
        return value
