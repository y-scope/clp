"""Garbage-collect cached search results."""

import asyncio
from contextlib import closing
from typing import Any, cast, Final

import pymongo
import pymongo.database
from clp_py_utils.clp_config import (
    ClpConfig,
    Database,
    QUERY_JOBS_TABLE_NAME,
    ResultsCache,
)
from clp_py_utils.clp_logging import configure_logging, get_logger
from clp_py_utils.sql_adapter import SqlAdapter

from job_orchestration.garbage_collector.constants import (
    MIN_TO_SECONDS,
    SEARCH_RESULT_GARBAGE_COLLECTOR_NAME,
)
from job_orchestration.scheduler.constants import QueryJobStatus

# Constants
MONGODB_ID_KEY: Final[str] = "_id"
MAX_NUM_JOB_IDS_PER_QUERY: Final[int] = 1000

logger = get_logger(SEARCH_RESULT_GARBAGE_COLLECTOR_NAME)


def _get_expired_job_ids(
    database_config: Database, job_ids: list[int], retention_period_minutes: int
) -> list[int]:
    """
    Filter query-job IDs by whether their retention periods have ended.

    MariaDB computes each query's completion time as `start_time + duration`. A query-job ID is
    included when the time since completion is greater than `retention_period_minutes`. For a
    terminated query without a completion time, `creation_time` is used instead.

    :param database_config: Configuration for the orchestration database.
    :param job_ids: Query-job IDs to filter.
    :param retention_period_minutes: Length of the retention period following query completion, in
        minutes.
    :return: Query-job IDs completed more than `retention_period_minutes` ago.
    """
    if len(job_ids) == 0:
        return []

    expired_job_ids: list[int] = []
    sql_adapter = SqlAdapter(database_config)
    with (
        closing(sql_adapter.create_connection(True)) as db_conn,
        closing(db_conn.cursor()) as db_cursor,
    ):
        for begin_idx in range(0, len(job_ids), MAX_NUM_JOB_IDS_PER_QUERY):
            job_ids_batch = job_ids[begin_idx : begin_idx + MAX_NUM_JOB_IDS_PER_QUERY]
            job_id_placeholders = ",".join(["%s"] * len(job_ids_batch))
            query = f"""
                SELECT id
                FROM `{QUERY_JOBS_TABLE_NAME}`
                WHERE id IN ({job_id_placeholders})
                AND (
                    TIMESTAMPADD(
                        MICROSECOND,
                        CAST(duration * 1000000 AS SIGNED),
                        start_time
                    ) < TIMESTAMPADD(MINUTE, %s, CURRENT_TIMESTAMP(3))
                    OR (
                        (start_time IS NULL OR duration IS NULL)
                        AND status IN (
                            {QueryJobStatus.SUCCEEDED},
                            {QueryJobStatus.FAILED},
                            {QueryJobStatus.CANCELLED},
                            {QueryJobStatus.KILLED}
                        )
                        AND creation_time < TIMESTAMPADD(MINUTE, %s, CURRENT_TIMESTAMP(3))
                    )
                )
                """
            db_cursor.execute(
                query,
                [*job_ids_batch, -retention_period_minutes, -retention_period_minutes],
            )
            rows = cast("list[tuple[int]]", db_cursor.fetchall())
            expired_job_ids.extend(row[0] for row in rows)
    return expired_job_ids


def _delete_result_metadata(
    database: pymongo.database.Database, results_metadata_collection_name: str, job_id: str
) -> None:
    results_metadata_collection = database.get_collection(results_metadata_collection_name)
    results_metadata_collection.delete_one({MONGODB_ID_KEY: job_id})


def _collect_and_sweep_expired_search_results(
    result_cache_config: ResultsCache,
    database_config: Database,
    results_metadata_collection_name: str,
) -> None:
    """
    Remove search results whose query completion time is older than the retention cutoff.

    Numeric MongoDB collection names are interpreted as query-job IDs. Collections selected by
    `_get_expired_job_ids` are dropped along with their result metadata documents.

    :param result_cache_config: MongoDB result-cache and retention configuration.
    :param database_config: Configuration for the orchestration database.
    :param results_metadata_collection_name: Name of the result metadata collection.
    """
    retention_period = result_cache_config.retention_period
    if retention_period is None:
        return

    deleted_job_ids: list[int] = []
    results_cache_client: pymongo.MongoClient[dict[str, Any]] = pymongo.MongoClient(
        result_cache_config.get_uri()
    )
    with results_cache_client:
        results_cache_db = results_cache_client.get_default_database()
        job_ids = [int(name) for name in results_cache_db.list_collection_names() if name.isdigit()]
        expired_job_ids = _get_expired_job_ids(database_config, job_ids, retention_period)
        for job_id in expired_job_ids:
            job_id_str = str(job_id)
            _delete_result_metadata(results_cache_db, results_metadata_collection_name, job_id_str)
            results_cache_db.get_collection(job_id_str).drop()
            deleted_job_ids.append(job_id)

    if len(deleted_job_ids) != 0:
        logger.debug("Deleted search results of job(s): %s.", deleted_job_ids)
    else:
        logger.debug("No search results matched the expiry criteria.")


async def search_result_garbage_collector(clp_config: ClpConfig) -> None:
    """Run search-result collection and sweeping at the configured interval."""
    configure_logging(logger, SEARCH_RESULT_GARBAGE_COLLECTOR_NAME)

    sweep_interval_secs = clp_config.garbage_collector.sweep_interval.search_result * MIN_TO_SECONDS

    logger.info("%s started.", SEARCH_RESULT_GARBAGE_COLLECTOR_NAME)
    try:
        while True:
            _collect_and_sweep_expired_search_results(
                clp_config.results_cache,
                clp_config.database,
                clp_config.webui.results_metadata_collection_name,
            )
            await asyncio.sleep(sweep_interval_secs)
    except Exception:
        logger.exception("%s exited with failure.", SEARCH_RESULT_GARBAGE_COLLECTOR_NAME)
        raise
