import asyncio
import multiprocessing
import time
from contextlib import closing

import msgpack
from clp_py_utils.clp_config import (
    QUERY_JOBS_TABLE_NAME,
)
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.scheduler.constants import QueryJobStatus, QueryJobType
from job_orchestration.scheduler.scheduler_data import QueryJobConfig


async def run_function_in_process(function, *args, initializer=None, init_args=None):
    """
    Runs the given function in a separate process wrapped in a *cancellable*
    asyncio task. This is necessary because asyncio's multiprocessing process
    cannot be cancelled once it's started.
    :param function: Method to run
    :param args: Arguments for the method
    :param initializer: Initializer for each process in the pool
    :param init_args: Arguments for the initializer
    :return: Return value of the method
    """
    pool = multiprocessing.Pool(1, initializer, init_args)

    loop = asyncio.get_event_loop()
    fut = loop.create_future()

    def process_done_callback(obj):
        loop.call_soon_threadsafe(fut.set_result, obj)

    def process_error_callback(err):
        loop.call_soon_threadsafe(fut.set_exception, err)

    pool.apply_async(
        function, args, callback=process_done_callback, error_callback=process_error_callback
    )

    try:
        return await fut
    except asyncio.CancelledError:
        pass
    finally:
        pool.terminate()
        pool.close()


def submit_query_job(
    sql_adapter: SQL_Adapter, job_config: QueryJobConfig, job_type: QueryJobType
) -> int:
    """
    Submits a query job.
    :param sql_adapter:
    :param job_config:
    :param job_type:
    :return: The job's ID.
    """
    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        # Create job
        db_cursor.execute(
            f"INSERT INTO `{QUERY_JOBS_TABLE_NAME}` (`job_config`, `type`) VALUES (%s, %s)",
            (msgpack.packb(job_config.dict()), job_type),
        )
        db_conn.commit()
        return db_cursor.lastrowid


def wait_for_query_job(sql_adapter: SQL_Adapter, job_id: int) -> QueryJobStatus:
    """
    Waits for the query job with the given ID to complete.
    :param sql_adapter:
    :param job_id:
    :return: The job's status on completion.
    """
    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        # Wait for the job to be marked complete
        while True:
            db_cursor.execute(
                f"SELECT `status` FROM `{QUERY_JOBS_TABLE_NAME}` WHERE `id` = {job_id}"
            )
            # There will only ever be one row since it's impossible to have more than one job with
            # the same ID
            new_status = QueryJobStatus(db_cursor.fetchall()[0]["status"])
            db_conn.commit()
            if new_status in (
                QueryJobStatus.SUCCEEDED,
                QueryJobStatus.FAILED,
                QueryJobStatus.CANCELLED,
            ):
                return new_status

            time.sleep(0.5)
