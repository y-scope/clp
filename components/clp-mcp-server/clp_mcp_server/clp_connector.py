"""ClpConnector: A class to interact with the CLP database and results cache."""

import asyncio
from typing import Any

import aiomysql
import msgpack
from clp_py_utils.clp_config import CLP_DEFAULT_DATASET_NAME, ClpDbNameType
from pymongo import AsyncMongoClient

from clp_mcp_server.constants import (
    POLLING_INTERVAL_SECONDS,
    QueryJobStatus,
    QueryJobType,
    SEARCH_MAX_NUM_RESULTS,
)
from clp_mcp_server.settings import CLP_DB_PASS, CLP_DB_USER


class ClpConnector:
    """A connector class to interact with the CLP database and results cache."""

    def __init__(self, clp_config: Any) -> None:
        """Initializes the ClpConnector with MongoDB and MariaDB configurations."""
        mongo_url = f"mongodb://{clp_config.results_cache.host}:{clp_config.results_cache.port}/"
        mongo_client = AsyncMongoClient(mongo_url)
        self._results_cache = mongo_client[clp_config.results_cache.db_name]

        # Configuration to be used in `aiomysql.connect` to MariaDB.
        self._db_conf = {
            "host": clp_config.database.host,
            "port": clp_config.database.port,
            "user": CLP_DB_USER,
            "password": CLP_DB_PASS,
            "db": clp_config.database.names[ClpDbNameType.CLP],
        }

        self._webui_addr = f"http://{clp_config.webui.host}:{clp_config.webui.port}"

    async def submit_query(
        self, query: str, begin_ts: int | None = None, end_ts: int | None = None
    ) -> str:
        """
        Submits a query to the CLP database and returns the ID of the query.

        :param query: The query string.
        :param begin_ts: The beginning timestamp of the query range.
        :param end_ts: The end timestamp of the query range.
        :return: The ID assigned to the query.
        :raise: ValueError if `end_ts` is smaller than `begin_ts`.
        :raise: RuntimeError if it fails to retrieve the ID of the submitted query.
        :raise: aiomysql.Error if there is an error connecting to or querying MariaDB.
        :raise: pymongo.errors.PyMongoError if there is an error interacting with MongoDB.
        :raise: Exception for any other unexpected errors.
        """
        if begin_ts is not None and end_ts is not None and end_ts < begin_ts:
            err_msg = f"end_ts {end_ts} is smaller than begin_ts {begin_ts}."
            raise ValueError(err_msg)

        job_config = msgpack.packb(
            {
                "begin_timestamp": begin_ts,
                "dataset": CLP_DEFAULT_DATASET_NAME,
                "end_timestamp": end_ts,
                "ignore_case": True,
                "max_num_results": SEARCH_MAX_NUM_RESULTS,
                "query_string": query,
            }
        )

        async with aiomysql.connect(**self._db_conf) as conn, conn.cursor() as cur:
            await cur.execute(
                "INSERT INTO query_jobs (type, job_config) VALUES (%s, %s);",
                (int(QueryJobType.SEARCH_OR_AGGREGATION), job_config),
            )
            await conn.commit()
            await cur.execute("SELECT LAST_INSERT_ID();")
            result = await cur.fetchone()
            query_id = str(result[0]) if result else None

        if query_id is None:
            err_msg = "Failed to retrieve the ID of the submitted query."
            raise RuntimeError(err_msg)

        await self._results_cache.create_collection(query_id)

        results_metadata_doc = {
            "_id": str(query_id),
            "errorMsg": None,
            "errorName": None,
            "lastSignal": "resp-querying",
            "queryEngine": "clp",
        }
        await self._results_cache["results-metadata"].insert_one(results_metadata_doc)

        return query_id

    async def read_job_status(self, query_id: str) -> QueryJobStatus:
        """
        Reads the job status of a query.

        :param query_id: The ID of the query.
        :return: The status of the query.
        :raise aiomysql.Error: If there is an error connecting to or querying MariaDB.
        :raise ValueError: When the query is not found.
        """
        async with aiomysql.connect(**self._db_conf) as conn, conn.cursor() as cur:
            await cur.execute("SELECT status FROM query_jobs WHERE id = %s;", (query_id,))
            result = await cur.fetchone()
            status = result[0] if result else None

        if status is None:
            err_msg = f"Query job with ID {query_id} not found."
            raise ValueError(err_msg)

        return status

    async def wait_query_completion(self, query_id: str, timeout: float | None = None) -> None:
        """
        Waits for the query to complete with an optional timeout.

        :param query_id: The ID of the query.
        :param timeout: Maximum time to wait in seconds, or None for no timeout.
        :raise: aiomysql.Error if there is an error connecting to or querying MariaDB.
        :raise: ValueError if the query is not found.
        :raise: RuntimeError if the query fails or is cancelled.
        :raise: TimeoutError if the timeout is reached before the query completes.
        """
        waiting_states = {QueryJobStatus.PENDING, QueryJobStatus.RUNNING, QueryJobStatus.CANCELLING}
        error_states = {QueryJobStatus.FAILED, QueryJobStatus.CANCELLED, QueryJobStatus.KILLED}
        event_loop = asyncio.get_running_loop()
        start_time = event_loop.time()

        while True:
            status = await self.read_job_status(query_id)
            if status == QueryJobStatus.SUCCEEDED:
                break
            if status in error_states:
                err_msg = (
                    f"Query job with ID {query_id} ended in status {QueryJobStatus(status).name}."
                )
                raise RuntimeError(err_msg)
            if status not in waiting_states:
                err_msg = f"Query job with ID {query_id} has unknown status {status}."
                raise RuntimeError(err_msg)

            if timeout and (event_loop.time() - start_time) > timeout:
                err_msg = f"Timeout waiting for query job with ID {query_id} to complete."
                raise TimeoutError(err_msg)

            await asyncio.sleep(POLLING_INTERVAL_SECONDS)

    async def read_results(self, query_id: str) -> list[dict]:
        """
        Reads the results of a query.

        :param query_id: The ID of the query.
        :return: A list of result documents.
        """
        collection = self._results_cache[str(query_id)]
        results = []

        async for doc in collection.find({}, limit=SEARCH_MAX_NUM_RESULTS):
            doc["link"] = (
                f"{self._webui_addr}/streamFile?type=json"
                f"&streamId={doc['archive_id']}"
                f"&dataset={CLP_DEFAULT_DATASET_NAME}"
                f"&logEventIdx={doc['log_event_ix']}"
            )
            doc["_id"] = None
            results.append(doc)

        return results
