from __future__ import annotations

import argparse
import asyncio
import logging
import multiprocessing
import pathlib
import sys
import time
from contextlib import closing

import msgpack
import pymongo
from clp_py_utils.clp_config import Database, ResultsCache, SEARCH_JOBS_TABLE_NAME
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.scheduler.constants import SearchJobStatus
from job_orchestration.scheduler.job_config import AggregationConfig, SearchConfig

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    get_clp_home,
    validate_and_load_config_file,
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


def create_and_monitor_job_in_db(
    db_config: Database,
    results_cache: ResultsCache,
    wildcard_query: str,
    tags: str | None,
    begin_timestamp: int | None,
    end_timestamp: int | None,
    ignore_case: bool,
    max_num_results: int,
    path_filter: str | None,
    do_count_aggregation: bool | None,
    count_by_time_bucket_size: int | None,
):
    search_config = SearchConfig(
        query_string=wildcard_query,
        begin_timestamp=begin_timestamp,
        end_timestamp=end_timestamp,
        ignore_case=ignore_case,
        max_num_results=max_num_results,
        path_filter=path_filter,
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
    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        # Create job
        db_cursor.execute(
            f"INSERT INTO `{SEARCH_JOBS_TABLE_NAME}` (`search_config`) VALUES (%s)",
            (msgpack.packb(search_config.dict()),),
        )
        db_conn.commit()
        job_id = db_cursor.lastrowid

        # Wait for the job to be marked complete
        while True:
            db_cursor.execute(
                f"SELECT `status` FROM `{SEARCH_JOBS_TABLE_NAME}` WHERE `id` = {job_id}"
            )
            # There will only ever be one row since it's impossible to have more than one job with
            # the same ID
            new_status = db_cursor.fetchall()[0]["status"]
            db_conn.commit()
            if new_status in (
                SearchJobStatus.SUCCEEDED,
                SearchJobStatus.FAILED,
                SearchJobStatus.CANCELLED,
            ):
                break

            time.sleep(0.5)

        with pymongo.MongoClient(results_cache.get_uri()) as client:
            search_results_collection = client[results_cache.db_name][str(job_id)]
            if max_num_results <= 0 or do_count_aggregation is not None:
                cursor = search_results_collection.find()
            else:
                cursor = (
                    search_results_collection.find().sort("timestamp", -1).limit(max_num_results)
                )

            if count_by_time_bucket_size is not None:
                for document in cursor:
                    print(f"timestamp: {document['timestamp']} count: {document['count']}")
            elif do_count_aggregation is not None:
                for document in cursor:
                    print(
                        f"tags: {document['group_tags']} count: {document['records'][0]['count']}"
                    )
            else:
                for document in cursor:
                    print(f"{document['original_path']}: {document['message']}", end="")


async def do_search(
    db_config: Database,
    results_cache: ResultsCache,
    wildcard_query: str,
    tags: str | None,
    begin_timestamp: int | None,
    end_timestamp: int | None,
    ignore_case: bool,
    max_num_results: int,
    path_filter: str | None,
    do_count_aggregation: bool | None,
    count_by_time_bucket_size: int | None,
):
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
            max_num_results,
            path_filter,
            do_count_aggregation,
            count_by_time_bucket_size,
        )
    )

    # Wait for the job to complete or an error to occur
    try:
        await db_monitor_task
    except asyncio.CancelledError:
        pass


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
    args_parser.add_argument(
        "--max-num-results",
        "-m",
        type=int,
        default=1000,
        help="Maximum number of latest results to return.",
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
        clp_config = validate_and_load_config_file(
            config_file_path, default_config_file_path, clp_home
        )
        clp_config.validate_logs_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    asyncio.run(
        do_search(
            clp_config.database,
            clp_config.results_cache,
            parsed_args.wildcard_query,
            parsed_args.tags,
            parsed_args.begin_time,
            parsed_args.end_time,
            parsed_args.ignore_case,
            parsed_args.max_num_results,
            parsed_args.file_path,
            parsed_args.count,
            parsed_args.count_by_time,
        )
    )

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
