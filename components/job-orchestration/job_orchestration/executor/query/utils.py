import datetime
import os
import signal
import subprocess
import sys
from logging import Logger
from typing import Any

from clp_py_utils.clp_config import QUERY_TASKS_TABLE_NAME
from clp_py_utils.sql_adapter import SqlAdapter

from job_orchestration.scheduler.scheduler_data import QueryTaskResult, QueryTaskStatus


def report_task_failure(
    sql_adapter: SqlAdapter,
    task_id: int,
    start_time: datetime.datetime,
):
    task_status = QueryTaskStatus.FAILED
    update_query_task_metadata(
        sql_adapter,
        task_id,
        dict(status=task_status, duration=0, start_time=start_time),
    )

    return QueryTaskResult(
        task_id=task_id,
        status=task_status,
        duration=0,
    ).model_dump()


def run_query_task(
    sql_adapter: SqlAdapter,
    logger: Logger,
    task_command: list[str],
    env_vars: dict[str, str] | None,
    task_name: str,
    job_id: str,
    task_id: int,
    start_time: datetime.datetime,
) -> tuple[QueryTaskResult, str]:
    task_status = QueryTaskStatus.RUNNING
    update_query_task_metadata(
        sql_adapter, task_id, dict(status=task_status, start_time=start_time)
    )

    logger.info(f"Running: {' '.join(task_command)}")
    task_proc = subprocess.Popen(
        task_command,
        preexec_fn=os.setpgrp,
        close_fds=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env_vars,
    )

    def sigterm_handler(_signo, _stack_frame):
        logger.debug("Entered sigterm handler")
        if task_proc.poll() is None:
            logger.debug(f"Trying to kill {task_name} process")
            # Kill the process group in case the task process also forked
            os.killpg(os.getpgid(task_proc.pid), signal.SIGTERM)
            os.waitpid(task_proc.pid, 0)
            logger.info(f"Cancelling {task_name} task.")
        # Add 128 to follow convention for exit codes from signals
        # https://tldp.org/LDP/abs/html/exitcodes.html#AEN23549
        sys.exit(_signo + 128)

    # Register the function to kill the child process at exit
    signal.signal(signal.SIGTERM, sigterm_handler)

    logger.info(f"Waiting for {task_name} to finish")
    # `communicate` is equivalent to `wait` in this case, but avoids deadlocks when piping to
    # stdout/stderr.
    stdout_data, stderr_data = task_proc.communicate()
    return_code = task_proc.returncode

    # Log stderr content (captured by Docker's fluentd driver)
    stderr_text = stderr_data.decode("utf-8").strip()
    if stderr_text:
        for line in stderr_text.split("\n"):
            logger.info(f"[{task_name} stderr] {line}")

    if 0 != return_code:
        task_status = QueryTaskStatus.FAILED
        logger.error(
            f"{task_name} task {task_id} failed for job {job_id} - return_code={return_code}"
        )
    else:
        task_status = QueryTaskStatus.SUCCEEDED
        logger.info(f"{task_name} task {task_id} completed for job {job_id}")

    duration = (datetime.datetime.now() - start_time).total_seconds()

    update_query_task_metadata(
        sql_adapter, task_id, dict(status=task_status, start_time=start_time, duration=duration)
    )

    task_result = QueryTaskResult(
        status=task_status,
        task_id=task_id,
        duration=duration,
    )

    if QueryTaskStatus.FAILED == task_status:
        task_result.error_log_path = "See worker logs (captured by Fluent Bit)"

    return task_result, stdout_data.decode("utf-8")


def update_query_task_metadata(
    sql_adapter: SqlAdapter,
    task_id: int,
    kv_pairs: dict[str, Any],
):
    with (
        closing(sql_adapter.create_connection(True)) as db_conn,
        closing(db_conn.cursor(dictionary=True)) as db_cursor,
    ):
        if not kv_pairs or len(kv_pairs) == 0:
            raise ValueError("No key-value pairs provided to update query task metadata")

        query = f"""
            UPDATE {QUERY_TASKS_TABLE_NAME}
            SET {", ".join([f'{k}="{v}"' for k, v in kv_pairs.items()])}
            WHERE id = {task_id}
        """
        db_cursor.execute(query)
