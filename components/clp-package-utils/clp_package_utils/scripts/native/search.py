"""Native search script for CLP."""

from __future__ import annotations

import argparse
import asyncio
import ipaddress
import logging
import pathlib
import socket
import sys
from typing import TYPE_CHECKING

import msgpack
import psutil
import pymongo
from clp_py_utils.clp_config import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    Database,
    ResultsCache,
)
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.scheduler.constants import QueryJobStatus, QueryJobType
from job_orchestration.scheduler.job_config import AggregationConfig, SearchJobConfig

from clp_package_utils.general import (
    get_clp_home,
    load_config_file,
)
from clp_package_utils.scripts.native.utils import (
    run_function_in_process,
    submit_query_job,
    validate_dataset_exists,
    wait_for_query_job,
)

if TYPE_CHECKING:
    from collections.abc import Callable

logger = logging.getLogger(__name__)


def create_and_monitor_job_in_db(
    db_config: Database,
    results_cache: ResultsCache,
    dataset: str | None,
    wildcard_query: str,
    tags: str | None,
    begin_timestamp: int | None,
    end_timestamp: int | None,
    ignore_case: bool,
    path_filter: str | None,
    network_address: tuple[str, int] | None,
    do_count_aggregation: bool | None,
    count_by_time_bucket_size: int | None,
) -> None:
    """
    Creates and monitors a search job in the database.

    :param db_config: Database configuration.
    :param results_cache: Results cache configuration.
    :param dataset: Dataset to search.
    :param wildcard_query: Search query string.
    :param tags: Tags to filter by.
    :param begin_timestamp: Start timestamp for search range.
    :param end_timestamp: End timestamp for search range.
    :param ignore_case: Whether to ignore case in search.
    :param path_filter: Path filter for search.
    :param network_address: Network address for results.
    :param do_count_aggregation: Whether to perform count aggregation.
    :param count_by_time_bucket_size: Time bucket size for count-by-time aggregation.
    """
    search_config = SearchJobConfig(
        dataset=dataset,
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
    with pymongo.MongoClient(results_cache.get_uri(), directConnection=True) as client:
        search_results_collection = client[results_cache.db_name][str(job_id)]
        if do_count_aggregation is not None:
            for document in search_results_collection.find():
                print(f"tags: {document['group_tags']} count: {document['records'][0]['count']}")  # noqa: T201
        elif count_by_time_bucket_size is not None:
            for document in search_results_collection.find():
                print(f"timestamp: {document['timestamp']} count: {document['count']}")  # noqa: T201

    if job_status != QueryJobStatus.SUCCEEDED:
        logger.error("job %s finished with unexpected status: %s", job_id, job_status)


def get_worker_connection_handler(
    raw_output: bool,
) -> Callable[[asyncio.StreamReader, asyncio.StreamWriter], None]:
    """
    Gets a connection handler for worker connections.

    :param raw_output: Whether to output raw results.
    :return: Async connection handler function.
    """
    async def worker_connection_handler(
        reader: asyncio.StreamReader, writer: asyncio.StreamWriter
    ) -> None:
        try:
            unpacker = msgpack.Unpacker()
            while True:
                # Read some data from the worker and feed it to msgpack
                buf = await reader.read(1024)
                if b"" == buf:
                    # Worker closed
                    return
                unpacker.feed(buf)

                # Print out any messages we can decode in the form of ORIG_PATH: MSG, or simply MSG
                # if raw output is enabled.
                for unpacked in unpacker:
                    if raw_output:
                        print(f"{unpacked[1]}", end="")  # noqa: T201
                    else:
                        print(f"{unpacked[2]}: {unpacked[1]}", end="")  # noqa: T201
        except asyncio.CancelledError:
            return
        finally:
            writer.close()

    return worker_connection_handler


async def do_search_without_aggregation(
    db_config: Database,
    results_cache: ResultsCache,
    dataset: str | None,
    wildcard_query: str,
    tags: str | None,
    begin_timestamp: int | None,
    end_timestamp: int | None,
    ignore_case: bool,
    path_filter: str | None,
    raw_output: bool,
) -> None:
    """
    Performs a search without aggregation.

    :param db_config: Database configuration.
    :param results_cache: Results cache configuration.
    :param dataset: Dataset to search.
    :param wildcard_query: Search query string.
    :param tags: Tags to filter by.
    :param begin_timestamp: Start timestamp for search range.
    :param end_timestamp: End timestamp for search range.
    :param ignore_case: Whether to ignore case in search.
    :param path_filter: Path filter for search.
    :param raw_output: Whether to output raw results.
    """
    host = _get_ipv4_address()
    if host is None:
        logger.error("Couldn't find an IPv4 address for receiving search results.")
        return
    logger.debug("Listening on %s for search results.", host)

    server = await asyncio.start_server(
        client_connected_cb=get_worker_connection_handler(raw_output),
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
            dataset,
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
    dataset: str | None,
    wildcard_query: str,
    tags: str | None,
    begin_timestamp: int | None,
    end_timestamp: int | None,
    ignore_case: bool,
    path_filter: str | None,
    do_count_aggregation: bool | None,
    count_by_time_bucket_size: int | None,
    raw_output: bool,
) -> None:
    """
    Performs a search with optional aggregation.

    :param db_config: Database configuration.
    :param results_cache: Results cache configuration.
    :param dataset: Dataset to search.
    :param wildcard_query: Search query string.
    :param tags: Tags to filter by.
    :param begin_timestamp: Start timestamp for search range.
    :param end_timestamp: End timestamp for search range.
    :param ignore_case: Whether to ignore case in search.
    :param path_filter: Path filter for search.
    :param do_count_aggregation: Whether to perform count aggregation.
    :param count_by_time_bucket_size: Time bucket size for count-by-time aggregation.
    :param raw_output: Whether to output raw results.
    """
    if do_count_aggregation is None and count_by_time_bucket_size is None:
        await do_search_without_aggregation(
            db_config,
            results_cache,
            dataset,
            wildcard_query,
            tags,
            begin_timestamp,
            end_timestamp,
            ignore_case,
            path_filter,
            raw_output,
        )
    else:
        await run_function_in_process(
            create_and_monitor_job_in_db,
            db_config,
            results_cache,
            dataset,
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


def main(argv: list[str]) -> int:
    """
    Searches compressed logs using CLP's native search.

    :param argv: Command-line arguments.
    :return: Exit code.
    """
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Searches the compressed logs.")
    args_parser.add_argument("--config", "-c", required=True, help="CLP configuration file.")
    args_parser.add_argument(
        "--verbose",
        "-v",
        action="store_true",
        help="Enable debug logging.",
    )
    args_parser.add_argument("wildcard_query", help="Wildcard query.")
    args_parser.add_argument(
        "--dataset",
        type=str,
        default=None,
        help="The dataset that the archives belong to.",
    )
    args_parser.add_argument(
        "-t", "--tags", help="Comma-separated list of tags of archives to search."
    )
    args_parser.add_argument(
        "--begin-time",
        type=int,
        help="Time range filter lower-bound (inclusive) as milliseconds from the UNIX epoch.",
    )
    args_parser.add_argument(
        "--end-time",
        type=int,
        help="Time range filter upper-bound (inclusive) as milliseconds from the UNIX epoch.",
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
    args_parser.add_argument(
        "--raw", action="store_true", help="Output the search results as raw logs."
    )
    parsed_args = args_parser.parse_args(argv[1:])
    if parsed_args.verbose:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    if (
        parsed_args.begin_time is not None
        and parsed_args.end_time is not None
        and parsed_args.begin_time > parsed_args.end_time
    ):
        msg = "begin_time > end_time"
        raise ValueError(msg)

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_logs_dir()
        clp_config.database.load_credentials_from_env()
    except Exception:
        logger.exception("Failed to load config.")
        return -1

    database_config: Database = clp_config.database
    dataset = parsed_args.dataset
    if dataset is not None:
        try:
            validate_dataset_exists(database_config, dataset)
        except Exception:
            logger.exception("Failed to validate dataset.")
            return -1

    try:
        asyncio.run(
            do_search(
                database_config,
                clp_config.results_cache,
                dataset,
                parsed_args.wildcard_query,
                parsed_args.tags,
                parsed_args.begin_time,
                parsed_args.end_time,
                parsed_args.ignore_case,
                parsed_args.file_path,
                parsed_args.count,
                parsed_args.count_by_time,
                parsed_args.raw,
            )
        )
    except asyncio.CancelledError:
        logger.exception("Search cancelled.")
        return -1

    return 0


def _get_ipv4_address() -> str | None:
    """
    Retrieves an IPv4 address of the host for network communication.

    :returns: The first non-loopback IPv4 address it finds.
    If no non-loopback address is available, returns the first loopback IPv4 address.
    If no IPv4 address is found, returns None.
    """
    fallback_ip = None

    for addresses in psutil.net_if_addrs().values():
        for addr in addresses:
            if addr.family != socket.AF_INET:
                continue
            ip = addr.address
            if not ipaddress.ip_address(ip).is_loopback:
                return ip
            if fallback_ip is None:
                fallback_ip = ip

    if fallback_ip is not None:
        logger.warning("Couldn't find a non-loopback IP address for receiving search results.")
    return fallback_ip


if "__main__" == __name__:
    sys.exit(main(sys.argv))
