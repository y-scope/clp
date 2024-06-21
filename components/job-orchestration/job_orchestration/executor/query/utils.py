from typing import Any, Dict
from pathlib import Path
from clp_py_utils.clp_config import QUERY_TASKS_TABLE_NAME
def update_query_task_metadata(
    db_cursor,
    task_id: int,
    kv_pairs: Dict[str, Any],
):
    if not kv_pairs or len(kv_pairs) == 0:
        raise ValueError("No key-value pairs provided to update query task metadata")

    query = f"""
        UPDATE {QUERY_TASKS_TABLE_NAME}
        SET {', '.join([f'{k}="{v}"' for k, v in kv_pairs.items()])}
        WHERE id = {task_id}
    """
    db_cursor.execute(query)

def get_logger_file_path(
    clp_logs_dir: Path,
    job_id: str,
    task_id: int
) -> Path:
    worker_logs_dir = clp_logs_dir / job_id
    worker_logs_dir.mkdir(exist_ok=True, parents=True)
    return worker_logs_dir / f"{task_id}-clo.log"
