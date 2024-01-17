from typing import Any, Dict

from celery.app.task import Task
from job_orchestration.executor.search.celery import app


@app.task(bind=True)
def fs_search(
    task: Task,
    job_id_str: str,
    output_config: Dict[str, Any],
    archive_id: str,
    timestamp_key: str,
    query: dict,
    db_uri: str
) -> Dict[str, Any]:
    return {}