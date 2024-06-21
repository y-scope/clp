from contextlib import closing
from pathlib import Path
from typing import Any, Dict

from clp_py_utils.clp_config import QUERY_TASKS_TABLE_NAME
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.scheduler.scheduler_data import QueryTaskResult, QueryTaskStatus


def get_logger_file_path(clp_logs_dir: Path, job_id: str, task_id: int) -> Path:
    worker_logs_dir = clp_logs_dir / job_id
    worker_logs_dir.mkdir(exist_ok=True, parents=True)
    return worker_logs_dir / f"{task_id}-clo.log"


def generate_final_task_results(
    task_id: int, job_status: QueryTaskStatus, duration: float, clo_log_path: Path
) -> Dict[Any, Any]:
    task_result = QueryTaskResult(
        status=job_status,
        task_id=task_id,
        duration=duration,
    )

    if QueryTaskStatus.FAILED == job_status:
        task_result.error_log_path = str(clo_log_path)

    return task_result.dict()


def update_query_task_metadata(
    sql_adapter: SQL_Adapter,
    task_id: int,
    kv_pairs: Dict[str, Any],
):
    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        if not kv_pairs or len(kv_pairs) == 0:
            raise ValueError("No key-value pairs provided to update query task metadata")

        query = f"""
            UPDATE {QUERY_TASKS_TABLE_NAME}
            SET {', '.join([f'{k}="{v}"' for k, v in kv_pairs.items()])}
            WHERE id = {task_id}
        """
        db_cursor.execute(query)
