"""CLPConnector: A class to interact with the CLP database and results cache."""

import asyncio

import aiomysql
import msgpack
from pymongo import AsyncMongoClient

from .constants import (
    POLLING_INTERVAL_SECONDS,
    QueryJobStatus,
    QueryJobType,
    SEARCH_MAX_NUM_RESULTS,
)
from .settings import (
    CLP_DB_NAME,
    CLP_DB_PASS,
    CLP_DB_PORT,
    CLP_DB_SERVICE_NAME,
    CLP_DB_USER,
    CLP_RESULTS_CACHE_CLP_DB_NAME,
    CLP_RESULTS_CACHE_PORT,
    CLP_RESULTS_CACHE_SERVICE_NAME,
)


class CLPConnector:
    """A connector class to interact with the CLP database and results cache."""

    def __init__(self) -> None:
        """Initializes the CLPConnector with MongoDB and MariaDB configurations."""
        mongo_url = f"mongodb://{CLP_RESULTS_CACHE_SERVICE_NAME}:{CLP_RESULTS_CACHE_PORT}/"
        self.mongo_client = AsyncMongoClient(mongo_url)
        self.results_cache = self.mongo_client[CLP_RESULTS_CACHE_CLP_DB_NAME]

        # Configuration to be used in `aiomysql.connect` to MariaDB.
        self.db_conf = {
            "host": CLP_DB_SERVICE_NAME,
            "port": CLP_DB_PORT,
            "user": CLP_DB_USER,
            "password": CLP_DB_PASS,
            "db": CLP_DB_NAME,
        }

    async def submit_query(self, query: str, begin_ts: int, end_ts: int) -> str:
        """
        Submits a query to the CLP database and returns the ID of the query.

        :param query: The query string.
        :type query: str
        :param begin_ts: The beginning timestamp of the query range.
        :type begin_ts: int
        :param end_ts: The end timestamp of the query range.
        :type end_ts: int

        :raises ValueError: If ``end_ts`` is smaller than ``begin_ts``.
        :raises aiomysql.Error: If there is an error connecting to or querying MariaDB.
        :raises pymongo.errors.PyMongoError: If there is an error interacting with MongoDB.
        :raises Exception: For any other unexpected errors.

        :returns: The ID assigned to the query.
        :rtype: str

        """
        if end_ts < begin_ts:
            err_msg = f"end_ts {end_ts} is smaller than begin_ts {begin_ts}."
            raise ValueError(err_msg)

        job_config = msgpack.packb(
            {
                "begin_timestamp": begin_ts,
                "dataset": None,
                "end_timestamp": end_ts,
                "ignore_case": True,
                "max_num_results": SEARCH_MAX_NUM_RESULTS,
                "query_string": query,
            }
        )

        async with aiomysql.connect(**self.db_conf) as conn, conn.cursor() as cur:
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

        await self.results_cache.create_collection(query_id)

        results_metadata_doc = {
            "_id": str(query_id),
            "errorMsg": None,
            "errorName": None,
            "lastSignal": "resp-querying",
            "queryEngine": "clp",
        }
        await self.results_cache["results-metadata"].insert_one(results_metadata_doc)

        return query_id

    async def read_job_status(self, query_id: str) -> int:
        """
        Reads the job status of a query.

        :param query_id: The ID of the query.
        :type query_id: str

        :raises aiomysql.Error: If there is an error connecting to or querying MariaDB.
        :raises ValueError: When the query is not found.

        :return: The status of the query.
        :rtype: int

        """
        async with aiomysql.connect(**self.db_conf) as conn, conn.cursor() as cur:
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
        :type query_id: str
        :param timeout: Maximum time to wait in seconds, or None for no timeout.
        :type timeout: float | None

        :raises aiomysql.Error: If there is an error connecting to or querying MariaDB.
        :raises ValueError: When the query is not found.
        :raises RuntimeError: When the query fails or is cancelled.

        """
        waiting_states = {QueryJobStatus.PENDING, QueryJobStatus.RUNNING, QueryJobStatus.CANCELLING}
        start_time = asyncio.get_event_loop().time()

        while True:
            status = await self.read_job_status(query_id)
            if status in waiting_states:
                await asyncio.sleep(POLLING_INTERVAL_SECONDS)
                if timeout and (asyncio.get_event_loop().time() - start_time) > timeout:
                    err_msg = f"Timeout waiting for query job with ID {query_id} to complete."
                    raise TimeoutError(err_msg)
            elif status == QueryJobStatus.SUCCEEDED:
                break
            elif status == QueryJobStatus.FAILED:
                err_msg = f"Query job with ID {query_id} failed."
                raise RuntimeError(err_msg)
            elif status == QueryJobStatus.CANCELLED:
                err_msg = f"Query job with ID {query_id} was cancelled."
                raise RuntimeError(err_msg)
            elif status == QueryJobStatus.KILLED:
                err_msg = f"Query job with ID {query_id} was killed."
                raise RuntimeError(err_msg)
            else:
                err_msg = f"Query job with ID {query_id} has unknown status {status}."
                raise RuntimeError(err_msg)

    async def read_results(self, query_id: str) -> list[dict]:
        """
        Reads the results of a query.

        :param query_id: The ID of the query.
        :type query_id: str

        :return: A list of result documents.
        :rtype: list[dict]

        """
        collection = self.results_cache[str(query_id)]
        results = []

        async for doc in collection.find({}, limit=SEARCH_MAX_NUM_RESULTS):
            results.append(doc)

        return results
