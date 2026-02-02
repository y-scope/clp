from __future__ import annotations

from contextlib import closing

from clp_py_utils.clp_config import (
    COMPRESSION_JOBS_TABLE_NAME,
    COMPRESSION_TASKS_TABLE_NAME,
    QUERY_JOBS_TABLE_NAME,
    QUERY_TASKS_TABLE_NAME,
)
from clp_py_utils.sql_adapter import SqlAdapter

from job_orchestration.scheduler.constants import (
    CompressionJobStatus,
    CompressionTaskStatus,
    QueryJobStatus,
    QueryTaskStatus,
    SchedulerType,
)


def kill_hanging_jobs(sql_adapter: SqlAdapter, scheduler_type: str) -> list[int] | None:
    if SchedulerType.COMPRESSION == scheduler_type:
        jobs_table_name = COMPRESSION_JOBS_TABLE_NAME
        job_status_running = CompressionJobStatus.RUNNING
        job_status_killed = CompressionJobStatus.KILLED
        tasks_table_name = COMPRESSION_TASKS_TABLE_NAME
        task_status_running = CompressionTaskStatus.RUNNING
        task_status_killed = CompressionTaskStatus.KILLED
    elif SchedulerType.QUERY == scheduler_type:
        jobs_table_name = QUERY_JOBS_TABLE_NAME
        job_status_running = QueryJobStatus.RUNNING
        job_status_killed = QueryJobStatus.KILLED
        tasks_table_name = QUERY_TASKS_TABLE_NAME
        task_status_running = QueryTaskStatus.RUNNING
        task_status_killed = QueryTaskStatus.KILLED
    else:
        raise ValueError(f"Unexpected scheduler type {scheduler_type}")

    with (
        closing(sql_adapter.create_connection()) as db_conn,
        closing(db_conn.cursor(dictionary=True)) as db_cursor,
    ):
        db_cursor.execute(
            f"""
            SELECT id
            FROM {jobs_table_name}
            WHERE status={job_status_running}
            """
        )
        hanging_job_ids = [row["id"] for row in db_cursor.fetchall()]
        num_hanging_jobs = len(hanging_job_ids)
        if 0 == num_hanging_jobs:
            return None

        job_id_placeholders_str = ",".join(["%s"] * len(hanging_job_ids))
        db_cursor.execute(
            f"""
            UPDATE {tasks_table_name}
            SET status={task_status_killed}, duration=0
            WHERE status={task_status_running}
            AND job_id IN ({job_id_placeholders_str})
            """,
            hanging_job_ids,
        )

        jobs_update_config = {"status": int(job_status_killed), "duration": 0}
        field_set_expressions = [f"{k} = %s" for k in jobs_update_config]
        if SchedulerType.COMPRESSION == scheduler_type:
            field_set_expressions.append("update_time = CURRENT_TIMESTAMP()")

        values = list(jobs_update_config.values()) + hanging_job_ids
        db_cursor.execute(
            f"""
            UPDATE {jobs_table_name}
            SET {", ".join(field_set_expressions)}
            WHERE id in ({job_id_placeholders_str})
            """,
            values,
        )
        db_conn.commit()
        return hanging_job_ids
