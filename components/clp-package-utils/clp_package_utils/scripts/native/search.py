from __future__ import annotations

import argparse
import asyncio
import ipaddress
import logging
import pathlib
import socket
import sys

import msgpack
import pymongo
from clp_py_utils.clp_config import Database, ResultsCache
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.scheduler.constants import QueryJobStatus, QueryJobType
from job_orchestration.scheduler.job_config import AggregationConfig, SearchJobConfig

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    get_clp_home,
    load_config_file,
)
from clp_package_utils.scripts.native.utils import (
    run_function_in_process,
    submit_query_job,
    wait_for_query_job,
)

# Setup logging
# Create logger
logger = logging.getLogger(__file__)
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] [%(name)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def create_and_monitor_job_in_db(
    db_config: Database,
    results_cache: ResultsCache,
    wildcard_query: str,
    tags: str | None,
    begin_timestamp: int | None,
    end_timestamp: int | None,
    ignore_case: bool,
    path_filter: str | None,
    network_address: tuple[str, int] | None,
    do_count_aggregation: bool | None,
    count_by_time_bucket_size: int | None,
):
    search_config = SearchJobConfig(
        query_string=wildcard_query,
        begin_timestamp=begin_timestamp,
        end_timestamp=end_timestamp,
        ignore_case=ignore_case,
        max_num_results=0,  # unlimited number of results
        path_filter=path_filter,
        network_address=network_address,
    )
    if do_count_aggregation is not None:
        search_config.aggregation_config = AggregationConfig(
            do_count_aggregation=do_count_aggregation
        )
    if count_by_time_bucket_size is not None:
        search_config.aggregation_config = AggregationConfig(
            count_by_time_bucket_size=count_by_time_bucket_size
        )
    if tags:
        tag_list = [tag.strip().lower() for tag in tags.split(",") if tag]
        if len(tag_list) > 0:
            search_config.tags = tag_list

    sql_adapter = SQL_Adapter(db_config)
    job_id = submit_query_job(sql_adapter, search_config, QueryJobType.SEARCH_OR_AGGREGATION)
    job_status = wait_for_query_job(sql_adapter, job_id)

    if do_count_aggregation is None and count_by_time_bucket_size is None:
        return
    with pymongo.MongoClient(results_cache.get_uri()) as client:
        search_results_collection = client[results_cache.db_name][str(job_id)]
        if do_count_aggregation is not None:
            for document in search_results_collection.find():
                print(f"tags: {document['group_tags']} count: {document['records'][0]['count']}")
        elif count_by_time_bucket_size is not None:
            for document in search_results_collection.find():
                print(f"timestamp: {document['timestamp']} count: {document['count']}")

    if job_status != QueryJobStatus.SUCCEEDED:
        logger.error(f"job {job_id} finished with unexpected status: {job_status}")


async def worker_connection_handler(reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
    try:
        unpacker = msgpack.Unpacker()
        while True:
            # Read some data from the worker and feed it to msgpack
            buf = await reader.read(1024)
            if b"" == buf:
                # Worker closed
                return
            unpacker.feed(buf)

            # Print out any messages we can decode in the form of ORIG_PATH: MSG
            for unpacked in unpacker:
                print(f"{unpacked[2]}: {unpacked[1]}", end="")
    except asyncio.CancelledError:
        return
    finally:
        writer.close()


async def do_search_without_aggregation(
    db_config: Database,
    results_cache: ResultsCache,
    wildcard_query: str,
    tags: str | None,
    begin_timestamp: int | None,
    end_timestamp: int | None,
    ignore_case: bool,
    path_filter: str | None,
):
    ip_list = socket.gethostbyname_ex(socket.gethostname())[2]
    if len(ip_list) == 0:
        logger.error("Couldn't determine the current host's IP.")
        return

    host = ip_list[0]
    for ip in ip_list:
        if ipaddress.ip_address(ip) not in ipaddress.IPv4Network("127.0.0.0/8"):
            host = ip
            break

    server = await asyncio.start_server(
        client_connected_cb=worker_connection_handler,
        host=host,
        port=0,
        family=socket.AF_INET,
    )

    port = int(server.sockets[0].getsockname()[1])
    server_task = asyncio.ensure_future(server.serve_forever())

    db_monitor_task = asyncio.ensure_future(
        run_function_in_process(
            create_and_monitor_job_in_db,
            db_config,
            results_cache,
            wildcard_query,
            tags,
            begin_timestamp,
            end_timestamp,
            ignore_case,
            path_filter,
            (host, port),
            None,
            None,
        )
    )

    # Wait for the job to complete or an error to occur
    pending = [server_task, db_monitor_task]
    try:
        done, pending = await asyncio.wait(pending, return_when=asyncio.FIRST_COMPLETED)
        if db_monitor_task in done:
            server.close()
            await server.wait_closed()
        else:
            logger.error("server_task completed unexpectedly.")
            try:
                server_task.result()
            except Exception:
                logger.exception("server_task failed.")
            db_monitor_task.cancel()
            await db_monitor_task
    except asyncio.CancelledError:
        server.close()
        await server.wait_closed()
        await db_monitor_task
        raise


async def do_search(
    db_config: Database,
    results_cache: ResultsCache,
    wildcard_query: str,
    tags: str | None,
    begin_timestamp: int | None,
    end_timestamp: int | None,
    ignore_case: bool,
    path_filter: str | None,
    do_count_aggregation: bool | None,
    count_by_time_bucket_size: int | None,
):
    if do_count_aggregation is None and count_by_time_bucket_size is None:
        await do_search_without_aggregation(
            db_config,
            results_cache,
            wildcard_query,
            tags,
            begin_timestamp,
            end_timestamp,
            ignore_case,
            path_filter,
        )
    else:
        await run_function_in_process(
            create_and_monitor_job_in_db,
            db_config,
            results_cache,
            wildcard_query,
            tags,
            begin_timestamp,
            end_timestamp,
            ignore_case,
            path_filter,
            None,
            do_count_aggregation,
            count_by_time_bucket_size,
        )


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Searches the compressed logs.")
    args_parser.add_argument("--config", "-c", required=True, help="CLP configuration file.")
    args_parser.add_argument("wildcard_query", help="Wildcard query.")
    args_parser.add_argument(
        "-t", "--tags", help="Comma-separated list of tags of archives to search."
    )
    args_parser.add_argument(
        "--begin-time",
        type=int,
        help="Time range filter lower-bound (inclusive) as milliseconds" " from the UNIX epoch.",
    )
    args_parser.add_argument(
        "--end-time",
        type=int,
        help="Time range filter upper-bound (inclusive) as milliseconds" " from the UNIX epoch.",
    )
    args_parser.add_argument(
        "--ignore-case",
        action="store_true",
        help="Ignore case distinctions between values in the query and the compressed data.",
    )
    args_parser.add_argument("--file-path", help="File to search.")
    args_parser.add_argument(
        "--count",
        action="store_const",
        help="Count the number of results.",
        const=True,
    )
    args_parser.add_argument(
        "--count-by-time",
        type=int,
        help="Count the number of results in each time span of the given size (ms).",
    )
    parsed_args = args_parser.parse_args(argv[1:])

    if (
        parsed_args.begin_time is not None
        and parsed_args.end_time is not None
        and parsed_args.begin_time > parsed_args.end_time
    ):
        raise ValueError("begin_time > end_time")

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_logs_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    try:
        asyncio.run(
            do_search(
                clp_config.database,
                clp_config.results_cache,
                parsed_args.wildcard_query,
                parsed_args.tags,
                parsed_args.begin_time,
                parsed_args.end_time,
                parsed_args.ignore_case,
                parsed_args.file_path,
                parsed_args.count,
                parsed_args.count_by_time,
            )
        )
    except asyncio.CancelledError:
        logger.error("Search cancelled.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
