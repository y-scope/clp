import argparse
import asyncio
import datetime
import logging
import multiprocessing
import pathlib
import socket
import sys
import time
from asyncio import StreamReader, StreamWriter
from contextlib import closing

import msgpack
import zstandard

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    validate_and_load_config_file,
    get_clp_home
)
from clp_py_utils.clp_config import CLP_METADATA_TABLE_PREFIX, Database
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.job_config import SearchConfig
from job_orchestration.scheduler.constants import JobStatus

# Setup logging
# Create logger
logger = logging.getLogger(__file__)
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] [%(name)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


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

    pool.apply_async(function, args, callback=process_done_callback, error_callback=process_error_callback)

    try:
        return await fut
    except asyncio.CancelledError:
        pass
    finally:
        pool.terminate()
        pool.close()


def create_and_monitor_job_in_db(db_config: Database, wildcard_query: str, path_filter: str,
                                 search_controller_host: str, search_controller_port: int):
    search_config = SearchConfig(
        search_controller_host=search_controller_host,
        search_controller_port=search_controller_port,
        wildcard_query=wildcard_query,
        path_filter=path_filter
    )

    sql_adapter = SQL_Adapter(db_config)
    zstd_cctx = zstandard.ZstdCompressor(level=3)
    with closing(sql_adapter.create_connection(True)) as db_conn, closing(db_conn.cursor(dictionary=True)) as db_cursor:
        # Create job
        db_cursor.execute(f"INSERT INTO `search_jobs` (`search_config`) VALUES (%s)",
                          (zstd_cctx.compress(msgpack.packb(search_config.dict())),))
        db_conn.commit()
        job_id = db_cursor.lastrowid

        # Create a task for each archive, in batches
        next_pagination_id = 0
        pagination_limit = 64
        num_tasks_added = 0
        while True:
            # Get next `limit` rows
            db_cursor.execute(
                f"""
                SELECT `id` FROM {CLP_METADATA_TABLE_PREFIX}archives 
                WHERE `pagination_id` >= {next_pagination_id} 
                LIMIT {pagination_limit}
                """
            )
            rows = db_cursor.fetchall()

            # Insert tasks
            db_cursor.execute(f"""
            INSERT INTO `search_tasks` (`job_id`, `archive_id`, `scheduled_time`) 
            VALUES ({"), (".join(f"{job_id}, '{row['id']}', '{datetime.datetime.utcnow()}'" for row in rows)})
            """)
            db_conn.commit()
            num_tasks_added += len(rows)

            if len(rows) < pagination_limit:
                # Less than limit rows returned, so there are no more rows
                break
            next_pagination_id += pagination_limit

        # Mark job as scheduled
        db_cursor.execute(f"""
        UPDATE `search_jobs`
        SET num_tasks={num_tasks_added}, status = '{JobStatus.SCHEDULED}'
        WHERE id = {job_id}
        """)
        db_conn.commit()

        # Wait for the job to be marked complete
        job_complete = False
        while not job_complete:
            db_cursor.execute(f"SELECT `status`, `status_msg` FROM `search_jobs` WHERE `id` = {job_id}")
            # There will only ever be one row since it's impossible to have more than one job with the same ID
            row = db_cursor.fetchall()[0]
            if JobStatus.SUCCEEDED == row['status']:
                job_complete = True
            elif JobStatus.FAILED == row['status']:
                logger.error(row['status_msg'])
                job_complete = True
            db_conn.commit()

            time.sleep(1)


async def worker_connection_handler(reader: StreamReader, writer: StreamWriter):
    try:
        unpacker = msgpack.Unpacker()
        while True:
            # Read some data from the worker and feed it to msgpack
            buf = await reader.read(1024)
            if b'' == buf:
                # Worker closed
                return
            unpacker.feed(buf)

            # Print out any messages we can decode
            for unpacked in unpacker:
                print(f"{unpacked[0]}: {unpacked[2]}", end='')
    except asyncio.CancelledError:
        return
    finally:
        writer.close()


async def do_search(db_config: Database, wildcard_query: str, path_filter: str, host: str):
    # Start server to receive and print results
    try:
        server = await asyncio.start_server(client_connected_cb=worker_connection_handler, host=host, port=0,
                                            family=socket.AF_INET)
    except asyncio.CancelledError:
        # Search cancelled
        return
    port = server.sockets[0].getsockname()[1]

    server_task = asyncio.ensure_future(server.serve_forever())

    db_monitor_task = asyncio.ensure_future(
        run_function_in_process(create_and_monitor_job_in_db, db_config, wildcard_query, path_filter, host, port))

    # Wait for the job to complete or an error to occur
    pending = [server_task, db_monitor_task]
    try:
        done, pending = await asyncio.wait(pending, return_when=asyncio.FIRST_COMPLETED)
        if db_monitor_task in done:
            server.close()
            await server.wait_closed()
        else:
            logger.error("server task unexpectedly returned")
            db_monitor_task.cancel()
            await db_monitor_task
    except asyncio.CancelledError:
        server.close()
        await server.wait_closed()
        await db_monitor_task


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Searches the compressed logs.")
    args_parser.add_argument('--config', '-c', required=True, help="CLP configuration file.")
    args_parser.add_argument('wildcard_query', help="Wildcard query.")
    args_parser.add_argument('--file-path', help="File to search.")
    parsed_args = args_parser.parse_args(argv[1:])

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = validate_and_load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_logs_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    # Get IP of local machine
    host_ip = None
    for ip in set(socket.gethostbyname_ex(socket.gethostname())[2]):
        host_ip = ip
        break
    if host_ip is None:
        logger.error("Could not determine IP of local machine.")
        return -1

    asyncio.run(do_search(clp_config.database, parsed_args.wildcard_query, parsed_args.file_path, host_ip))

    return 0


if '__main__' == __name__:
    sys.exit(main(sys.argv))
