from __future__ import annotations

import argparse
import datetime
import logging
import pathlib
import pymongo
import sys
import time
from contextlib import closing

import msgpack
import zstandard

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    validate_and_load_config_file,
    get_clp_home
)
from clp_py_utils.clp_config import (
    CLP_METADATA_TABLE_PREFIX,
    Database,
    ResultsCache
)
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


def do_search(db_config: Database, results_cache: ResultsCache, wildcard_query: str,
                    begin_timestamp: int | None, end_timestamp: int | None, path_filter: str):
    search_config = SearchConfig(
        wildcard_query=wildcard_query,
        begin_timestamp=begin_timestamp,
        end_timestamp=end_timestamp,
        path_filter=path_filter
    )

    sql_adapter = SQL_Adapter(db_config)
    zstd_cctx = zstandard.ZstdCompressor(level=3)
    with closing(sql_adapter.create_connection(True)) as \
            db_conn, closing(db_conn.cursor(dictionary=True)) as db_cursor:
        # Create job
        db_cursor.execute(f"INSERT INTO `search_jobs` (`search_config`) VALUES (%s)",
                          (zstd_cctx.compress(msgpack.packb(search_config.dict())),))
        db_conn.commit()
        job_id = db_cursor.lastrowid

        next_pagination_id = 0
        pagination_limit = 64
        num_tasks_added = 0
        query_base_conditions = []
        if begin_timestamp is not None:
            query_base_conditions.append(f"`end_timestamp` >= {begin_timestamp}")
        if end_timestamp is not None:
            query_base_conditions.append(f"`begin_timestamp` <= {end_timestamp}")
        while True:
            # Get next `limit` rows
            query_conditions = query_base_conditions + [f"`pagination_id` >= {next_pagination_id}"]
            query = f"""
                SELECT `id` FROM {CLP_METADATA_TABLE_PREFIX}archives 
                WHERE {" AND ".join(query_conditions)} 
                LIMIT {pagination_limit}
                """
            db_cursor.execute(query)
            rows = db_cursor.fetchall()
            if len(rows) == 0:
                break

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

            time.sleep(0.5)

        client = pymongo.MongoClient(results_cache.get_uri())
        search_results_collection = client[results_cache.db_name][str(job_id)]
        for document in search_results_collection.find():
            print(document)


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Searches the compressed logs.")
    args_parser.add_argument('--config', '-c', required=True, help="CLP configuration file.")
    args_parser.add_argument('wildcard_query', help="Wildcard query.")
    args_parser.add_argument('--begin-time', type=int,
                             help="Time range filter lower-bound (inclusive) as milliseconds"
                                  " from the UNIX epoch.")
    args_parser.add_argument('--end-time', type=int,
                             help="Time range filter upper-bound (inclusive) as milliseconds"
                                  " from the UNIX epoch.")
    args_parser.add_argument('--file-path', help="File to search.")
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
        clp_config = validate_and_load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_logs_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    do_search(clp_config.database, clp_config.results_cache, parsed_args.wildcard_query,
              parsed_args.begin_time, parsed_args.end_time, parsed_args.file_path)

    return 0


if '__main__' == __name__:
    sys.exit(main(sys.argv))
