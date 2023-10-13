import argparse
import logging
import pathlib
import sys
import time

import msgpack
import mysql.connector
import pymongo

from clp_py_utils.clp_config import CLPConfig, SEARCH_JOBS_TABLE_NAME
from clp_py_utils.core import read_yaml_config_file
from job_orchestration.scheduler.common import JobStatus

# Setup logging
# Create logger
logger = logging.getLogger(__file__)
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] [%(name)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def main(argv):
    args_parser = argparse.ArgumentParser(description="Searches the compressed logs.")
    args_parser.add_argument('--config', '-c', required=True, help="CLP configuration file.")
    args_parser.add_argument('wildcard_query', help="Wildcard query.")
    args_parser.add_argument('--count', help="Perform a count aggregation", action="store_true")
    parsed_args = args_parser.parse_args(argv[1:])

    config_path = pathlib.Path(parsed_args.config)
    # load config file
    try:
        clp_config = CLPConfig.parse_obj(read_yaml_config_file(config_path))
    except:
        logger.exception("Failed to load config.")
        return -1

    current_time = time.time()

    db_conn = mysql.connector.connect(
        host=clp_config.database.host,
        port=clp_config.database.port,
        database=clp_config.database.name,
        user=clp_config.database.username,
        password=clp_config.database.password,
    )

    wildcard_query = parsed_args.wildcard_query

    collection_name = clp_config.results_cache.results_collection_name
    query = {
        "pipeline_string": wildcard_query,
        "timestamp_begin": 0,
        "timestamp_end": sys.maxsize,
        "path_regex": '',
        "match_case": True,
        "results_collection_name": collection_name
    }
    if parsed_args.count:
        query["count"] = True

    cursor = db_conn.cursor()

    # Set up a connection to your MongoDB instance
    db_name = clp_config.results_cache.db_name
    client = pymongo.MongoClient(clp_config.results_cache.get_uri())
    search_results_collection = client[db_name][collection_name]
    # Delete all documents in the collection
    result = search_results_collection.delete_many({})

    sql = f"INSERT INTO {SEARCH_JOBS_TABLE_NAME} (search_config) VALUES (%s)"
    cursor.execute(sql, (msgpack.packb(query),))
    db_conn.commit()

    job_id = cursor.lastrowid

    prev_status = JobStatus.PENDING
    logger.info(f"Submitted job {job_id}")
    while True:
        cursor.execute(f"SELECT status FROM {SEARCH_JOBS_TABLE_NAME} WHERE id={job_id}")
        rows = cursor.fetchall()
        if len(rows) != 1:
            logger.error("Invalid database state, exiting...")
            break
        status, = rows[0]
        db_conn.commit()

        if prev_status != status:
            prev_status = status
            logger.info(f"Job {job_id} now {JobStatus(status).to_str()}")

        if not (status == JobStatus.PENDING or status == JobStatus.RUNNING or status == JobStatus.CANCELLING or status == JobStatus.PENDING_REDUCER or status == JobStatus.REDUCER_READY or status == JobStatus.PENDING_REDUCER_DONE):
            break

        time.sleep(0.5)
    end_time = time.time() - current_time
    logger.info(f"time elapsed: {end_time} seconds")

    # Retrieve and print all documents in the collection
    for document in search_results_collection.find():
        logger.info(document)

    # Close the MongoDB connection
    client.close()

    cursor.close()
    db_conn.close()


if "__main__" == __name__:
    sys.exit(main(sys.argv))
