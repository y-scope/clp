#!/usr/bin/env python3
import argparse
import calendar
import asyncio
import datetime
import logging
import multiprocessing
import os
import pathlib
import socket
import sys
import time
from asyncio import StreamReader, StreamWriter
from contextlib import closing
import threading
import json

import enum
import errno
import secrets
import subprocess
import typing

import yaml

from clp_py_utils.clp_config import CLPConfig, CLP_DEFAULT_CREDENTIALS_FILE_PATH
from clp_py_utils.core import \
    get_config_value, \
    make_config_path_absolute, \
    read_yaml_config_file, \
    validate_path_could_be_dir

# Setup logging
# Create logger
logger = logging.getLogger(__file__)
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] [%(name)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def get_clp_home():
    # Determine CLP_HOME from an environment variable or this script's path
    _clp_home = None
    if 'CLP_HOME' in os.environ:
        _clp_home = pathlib.Path(os.environ['CLP_HOME'])
    else:
        for path in pathlib.Path(__file__).resolve().parents:
            if 'sbin' == path.name:
                _clp_home = path.parent
                break

    if _clp_home is None:
        logger.error("CLP_HOME is not set and could not be determined automatically.")
        return None
    elif not _clp_home.exists():
        logger.error("CLP_HOME set to nonexistent path.")
        return None

    return _clp_home.resolve()


def load_bundled_python_lib_path(_clp_home):
    python_site_packages_path = _clp_home / 'packages'
    if not python_site_packages_path.is_dir():
        logger.error("Failed to load python3 packages bundled with CLP.")
        return False

    # Add packages to the front of the path
    sys.path.insert(0, str(python_site_packages_path))

    return True


clp_home = get_clp_home()
if clp_home is None or not load_bundled_python_lib_path(clp_home):
    sys.exit(-1)

import msgpack
import zstandard
CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH = pathlib.Path('etc') / 'clp-config.yml'
# from clp.package_utils import CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH, validate_and_load_config_file
from clp_py_utils.clp_config import CLP_METADATA_TABLE_PREFIX, Database
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.job_config import SearchConfig
from job_orchestration.scheduler.constants import JobStatus


def validate_and_load_config_file(config_file_path: pathlib.Path, default_config_file_path: pathlib.Path,
                                  clp_home: pathlib.Path):
    if config_file_path.exists():
        raw_clp_config = read_yaml_config_file(config_file_path)
        if raw_clp_config is None:
            clp_config = CLPConfig()
        else:
            clp_config = CLPConfig.parse_obj(raw_clp_config)
    else:
        if config_file_path != default_config_file_path:
            raise ValueError(f"Config file '{config_file_path}' does not exist.")

        clp_config = CLPConfig()

    clp_config.make_config_paths_absolute(clp_home)

    # Make data and logs directories node-specific
    hostname = socket.gethostname()
    clp_config.data_directory /= hostname
    clp_config.logs_directory /= hostname

    return clp_config


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


class Counter(object):

    def __init__(self, start = 0):
        self.lock = threading.Lock()
        self.value = start

    def increment(self):
        self.lock.acquire()
        try:
            self.value = self.value + 1
        finally:
            self.lock.release()

    def get(self):
        return self.value


counter = Counter()


def create_and_monitor_job_in_db(
        db_config: Database, wildcard_query: str, path_filter: str,
        search_controller_host: str, search_controller_port: int,
        pagination_limit: int, next_pagination_id: int, context):
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

        num_tasks_added = 0

        if context is not None:
            if context["unit"] == "HOUR":
                uppertlimit = calendar.timegm(datetime.datetime.utcnow().timetuple())
                lowertlimit = calendar.timegm(
                    (datetime.datetime.utcnow() - datetime.timedelta(hours=context["interval"])).timetuple())
            if context["unit"] == "MINUTE":
                uppertlimit = calendar.timegm(datetime.datetime.utcnow().timetuple())
                lowertlimit = calendar.timegm(
                    (datetime.datetime.utcnow() - datetime.timedelta(minutes=context["interval"])).timetuple())

        next_pagination_id += pagination_limit
        job_stmt = f"""
            select archive_id, DENSE_RANK() OVER (ORDER BY archive_id) as no 
            from clp_files 
            where begin_timestamp between 
            {lowertlimit*1000}
            and 
            {uppertlimit*1000} 
            group by archive_id limit {pagination_limit} offset {next_pagination_id};
            """
        db_cursor.execute(job_stmt)
        rows = db_cursor.fetchall()

        if len(rows) == 0:
            return -1

        stmt = f"""
        INSERT INTO `search_tasks` (`job_id`, `archive_id`, `scheduled_time`) 
        VALUES ({"), (".join(f"{job_id}, '{row['archive_id']}', '{datetime.datetime.utcnow()}'" for row in rows)})
        """

        db_cursor.execute(stmt)
        db_conn.commit()
        num_tasks_added += len(rows)

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
            time.sleep(1)
            db_cursor.execute(f"SELECT `status`, `status_msg` FROM `search_jobs` WHERE `id` = {job_id}")
            row = db_cursor.fetchall()[0]

            if JobStatus.SUCCEEDED == row['status']:
                job_complete = True
            elif JobStatus.FAILED == row['status']:
                logger.error(row['status_msg'])
                job_complete = True

            db_conn.commit()

    return 0


async def increment_results_counter():
    counter.increment()
    return 0


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

            for unpacked in unpacker:
                try:
                    log_with_time = unpacked[2]
                    application_log = log_with_time[log_with_time.find(" "):]
                    parsed_app_log = json.loads(application_log)
                    parsed_app_log = json.dumps(parsed_app_log, indent=4)
                    print(parsed_app_log)
                except:
                    print(f"{unpacked[0]}: {unpacked[2]}")

                task = asyncio.create_task(increment_results_counter())
                await task

    except asyncio.CancelledError:
        return
    finally:
        writer.close()


async def do_search(db_config: Database, wildcard_query: str, path_filter: str, host: str, time_context):
    # Start server to receive and print results
    try:
        server = await asyncio.start_server(client_connected_cb=worker_connection_handler, host=host, port=0,
                                            family=socket.AF_INET)
    except asyncio.CancelledError:
        # Search cancelled
        return
    port = server.sockets[0].getsockname()[1]
    server_task = asyncio.ensure_future(server.serve_forever())

    pagination_limit = 30
    next_pagination_id = -30

    while counter.get() <= 100:
        next_pagination_id += pagination_limit

        db_monitor_task = asyncio.ensure_future(
            run_function_in_process(
                create_and_monitor_job_in_db, db_config, wildcard_query,
                path_filter, host, port, pagination_limit, next_pagination_id, time_context))
        pending = [server_task, db_monitor_task]

        try:
            done, pending = await asyncio.wait(pending, return_when=asyncio.FIRST_COMPLETED)
            if db_monitor_task not in done or db_monitor_task.result() == -1:
                break
        except:
            pass

    try:
        server.close()
        await server.wait_closed()
    except asyncio.CancelledError:
        server.close()
        await server.wait_closed()


def main(argv):
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Searches the compressed logs.")
    args_parser.add_argument('--config', '-c', required=True, help="CLP configuration file.")
    args_parser.add_argument('--wildcard_query', required=True, help="Wildcard query.")
    args_parser.add_argument('--file-path', help="File to search.")
    args_parser.add_argument('--context', required=True, help="Time context to search")
    parsed_args = args_parser.parse_args(argv[1:])

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = validate_and_load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_logs_dir()
    except:
        print("failed to load config.")
        return -1

    # Get IP of local machine
    host_ip = None
    for ip in set(socket.gethostbyname_ex(socket.gethostname())[2]):
        host_ip = ip
        break
    if host_ip is None:
        print("could not determine ip of local machine.")
        return -1

    if parsed_args.context is not None:
        context = parse_context(parsed_args.context)
        if context == -1:
            print("invalid date context given.should be of something like 'last15m', 'last1h', etc")
            return
    else:
        context = None

    asyncio.run(do_search(clp_config.database, parsed_args.wildcard_query, parsed_args.file_path, host_ip, context))

    return 0


def parse_context(context):
    if len(context) < 6:
        return -1
    if context[:4] != "last":
        return -1
    if context[-1] == "h":
        unit = "HOUR"
    elif context[-1] == "m":
        unit = "MINUTE"
    else:
        return -1

    if not context[4:-1].isdigit():
        return -1

    value = int(context[4:-1])
    if value > 24 and unit == "HOUR":
        print("can only filter logs upto 24 hours")
        return -1

    if value > 60 and unit == "MINUTE":
        print("can filter logs upto 60 mins only. use hours instead")
        return -1

    ctx = {'unit': unit, 'interval': int(context[4:-1])}
    return ctx


if '__main__' == __name__:
    sys.exit(main(sys.argv))

