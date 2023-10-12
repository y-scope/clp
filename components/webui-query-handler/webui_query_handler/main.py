#!/usr/bin/env python3
import argparse
import asyncio
import enum
import functools
import json
import logging
import math
import multiprocessing
import os
import sys
import time
import typing

import msgpack
import mysql.connector
import pymongo
import pymongo.errors
import websockets
from pathlib import Path
from pydantic import BaseModel
from pymongo import ASCENDING, DESCENDING, IndexModel, MongoClient

from clp_py_utils.clp_config import SEARCH_JOBS_TABLE_NAME
from clp_py_utils.clp_logging import get_logging_level
from job_orchestration.scheduler.common import JobStatus

# Setup logging
# Create logger
logger = logging.getLogger()
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)

# Constants
GENERAL_METADATA_DOC_ID = "general"
TIMELINE_DOC_ID = "timeline"
NUM_SEARCH_RESULTS_DOC_ID = "num_search_results"


class DatabaseConnectionConfig(BaseModel):
    host: str
    port: int
    db_name: str
    user: str
    password: str


class ResultType(enum.IntEnum):
    MESSAGE_RESULT = 0


class ServerMessageType(enum.IntEnum):
    ERROR = 0
    OPERATION_COMPLETE = 1
    PREPARING_FOR_QUERY = 2
    QUERY_STARTED = 3


class ClientMessageType(enum.IntEnum):
    CANCEL_OPERATION = 0
    QUERY = 1
    CLEAR_RESULTS = 2
    UPDATE_TIMELINE_RANGE = 3


class HandlerState(enum.IntEnum):
    READY = 0
    CLEAR_RESULTS_IN_PROGRESS = 1
    CLEAR_RESULTS_IN_PROGRESS_BEFORE_QUERY = 2
    QUERY_IN_PROGRESS = 3


# Globals
g_webui_connected = {}


async def run_function_in_process(function, *args, initializer=None, init_args=None):
    """
    Runs the given function in a separate process wrapped in a cancellable asyncio task. This is necessary because asyncio's multiprocessing process cannot be
    cancelled once it's started.
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
        function,
        args,
        callback=process_done_callback,
        error_callback=process_error_callback,
    )

    try:
        return await fut
    except asyncio.CancelledError:
        pass
    finally:
        pool.terminate()
        pool.close()


def submit_query(
        db_conn_conf: DatabaseConnectionConfig,
        results_cache_uri: str,
        results_collection_name: str,
        query: dict,
):
    # Connect to database
    client = MongoClient(results_cache_uri)
    db = client.get_default_database()

    search_results_collection = pymongo.collection.Collection(
        db,
        results_collection_name,
        write_concern=pymongo.collection.WriteConcern(w=1, wtimeout=2000),
    )

    # Create timestamp index
    timestamp_ascending_index = IndexModel(
        [("timestamp", ASCENDING), ("_id", ASCENDING)], name="timestamp-ascending"
    )
    timestamp_descending_index = IndexModel(
        [("timestamp", DESCENDING), ("_id", DESCENDING)], name="timestamp-descending"
    )
    search_results_collection.create_indexes(
        [timestamp_ascending_index, timestamp_descending_index]
    )

    client.close()

    db_conn = mysql.connector.connect(
        host=db_conn_conf.host,
        port=db_conn_conf.port,
        database=db_conn_conf.db_name,
        user=db_conn_conf.user,
        password=db_conn_conf.password,
    )
    cursor = db_conn.cursor()

    sql = f"INSERT INTO {SEARCH_JOBS_TABLE_NAME} (search_config) VALUES (%s)"
    cursor.execute(sql, (msgpack.packb(query),))
    db_conn.commit()
    job_id = cursor.lastrowid

    cursor.close()
    db_conn.close()

    return job_id


def cancel_query(
        db_conn_conf: DatabaseConnectionConfig,
        job_id
):
    db_conn = mysql.connector.connect(
        host=db_conn_conf.host,
        port=db_conn_conf.port,
        database=db_conn_conf.db_name,
        user=db_conn_conf.user,
        password=db_conn_conf.password,
    )
    cursor = db_conn.cursor()

    sql = f"UPDATE {SEARCH_JOBS_TABLE_NAME} SET status = {JobStatus.CANCELLING} WHERE id={job_id}"
    cursor.execute(sql)
    db_conn.commit()

    cursor.close()
    db_conn.close()


def wait_for_query_to_complete(
    db_conn_conf: DatabaseConnectionConfig,
    job_id
):
    db_conn = mysql.connector.connect(
        host=db_conn_conf.host,
        port=db_conn_conf.port,
        database=db_conn_conf.db_name,
        user=db_conn_conf.user,
        password=db_conn_conf.password,
    )
    cursor = db_conn.cursor()

    prev_status = JobStatus.PENDING
    query_successful = True
    while True:
        cursor.execute(f"SELECT status FROM {SEARCH_JOBS_TABLE_NAME} WHERE id={job_id}")
        rows = cursor.fetchall()
        if len(rows) != 1:
            logger.error("Invalid database state, exiting...")
            query_successful = False
            break
        (status,) = rows[0]
        db_conn.commit()

        if prev_status != status:
            prev_status = status
            logger.info(
                f"Search job {job_id} status {JobStatus(prev_status)} -> {JobStatus(status)}")

        if not (
            status == JobStatus.PENDING
            or status == JobStatus.RUNNING
            or status == JobStatus.CANCELLING
            or status == JobStatus.PENDING_REDUCER
            or status == JobStatus.REDUCER_READY
            or status == JobStatus.PENDING_REDUCER_DONE
        ):
            break

        time.sleep(0.5)

    cursor.close()
    db_conn.close()

    return query_successful


async def receive_from_client(websocket) -> typing.Tuple[bool, typing.Optional[str]]:
    try:
        msg_str = await websocket.recv()
        msg = json.loads(msg_str)
        return True, msg
    except json.JSONDecodeError as ex:
        await send_error_msg(websocket, "Internal error - " + str(ex))
    except websockets.ConnectionClosed:
        # This is expected when the connection is closed
        logger.info("receive_from_client: Connection closed.")
    except ConnectionResetError:
        # This is expected when the connection is closed
        logger.info("receive_from_client: Connection reset.")
    except asyncio.IncompleteReadError as ex:
        if len(ex.partial):
            logger.error(
                'receive_from_client: Unexpected incomplete message "'
                + ex.partial.decode()
                + '".'
            )
        else:
            # This is expected when the connection is closed
            logger.info(
                "receive_from_client: Incomplete read (connection may have been closed)."
            )
    except asyncio.CancelledError:
        # This is expected when the task is cancelled
        logger.info("receive_from_client: Cancelled.")
    except:
        logger.exception("receive_from_client: Unexpected exception.")

    return False, None


def clear_results(
    db_uri: str, results_collection_name: str, results_metadata_collection_name: str
):
    # Connect to database
    client = MongoClient(db_uri)
    db = client.get_default_database()

    # Clear previous results
    search_results_collection = pymongo.collection.Collection(
        db,
        results_collection_name,
        write_concern=pymongo.collection.WriteConcern(w=1, wtimeout=2000),
    )
    search_results_collection.drop()
    search_results_metadata_collection = pymongo.collection.Collection(
        db, results_metadata_collection_name
    )
    search_results_metadata_collection.drop()
    return True


def update_timeline_and_count(
    search_results_collection: pymongo.collection.Collection,
    search_results_metadata_collection: pymongo.collection.Collection,
    time_range,
    output_result_type: ResultType,
):
    # Update count
    num_results = search_results_collection.count_documents({})
    search_results_metadata_collection.update_one(
        {"_id": NUM_SEARCH_RESULTS_DOC_ID}, {"$set": {"all": num_results}}, upsert=True
    )

    if (
        ResultType.MESSAGE_RESULT == output_result_type
    ):
        # Update timeline
        if time_range["begin"] is None:
            # Get time range of search results
            results_time_range_pipeline = [
                {
                    "$group": {
                        "_id": None,
                        "ts_min_ms": {"$min": "$timestamp"},
                        "ts_max_ms": {"$max": "$timestamp"},
                    }
                }
            ]
            time_range = {"begin": 0, "end": 0}
            for doc in search_results_collection.aggregate(
                pipeline=results_time_range_pipeline
            ):
                time_range["begin"] = doc["ts_min_ms"]
                time_range["end"] = doc["ts_max_ms"]

        # FIXME: may want to clear time chart
        if time_range["end"] == None or time_range["begin"] == None:
            return

        ts_range_ms = time_range["end"] - time_range["begin"]

        time_period_selections = [
            {"name": "1 sec", "milliseconds": 1000 * 1},
            {"name": "2 sec", "milliseconds": 1000 * 2},
            {"name": "5 sec", "milliseconds": 1000 * 5},
            {"name": "10 sec", "milliseconds": 1000 * 10},
            {"name": "15 sec", "milliseconds": 1000 * 15},
            {"name": "30 sec", "milliseconds": 1000 * 30},
            {"name": "1 min", "milliseconds": 1000 * 60 * 1},
            {"name": "2 min", "milliseconds": 1000 * 60 * 2},
            {"name": "5 min", "milliseconds": 1000 * 60 * 5},
            {"name": "10 min", "milliseconds": 1000 * 60 * 10},
            {"name": "15 min", "milliseconds": 1000 * 60 * 15},
            {"name": "20 min", "milliseconds": 1000 * 60 * 20},
            {"name": "30 min", "milliseconds": 1000 * 60 * 30},
            {"name": "1 hour", "milliseconds": 1000 * 60 * 60 * 1},
            {"name": "2 hours", "milliseconds": 1000 * 60 * 60 * 2},
            {"name": "3 hours", "milliseconds": 1000 * 60 * 60 * 3},
            {"name": "4 hours", "milliseconds": 1000 * 60 * 60 * 4},
            {"name": "8 hours", "milliseconds": 1000 * 60 * 60 * 8},
            {"name": "12 hours", "milliseconds": 1000 * 60 * 60 * 12},
            {"name": "1 day", "milliseconds": 1000 * 60 * 60 * 24 * 1},
            {"name": "2 days", "milliseconds": 1000 * 60 * 60 * 24 * 2},
            {"name": "5 days", "milliseconds": 1000 * 60 * 60 * 24 * 5},
            {"name": "15 days", "milliseconds": 1000 * 60 * 60 * 24 * 15},
            {"name": "1 month", "milliseconds": 1000 * 60 * 60 * 24 * 30 * 1},
            {"name": "2 months", "milliseconds": 1000 * 60 * 60 * 24 * 30 * 2},
            {"name": "3 months", "milliseconds": 1000 * 60 * 60 * 24 * 30 * 3},
            {"name": "4 months", "milliseconds": 1000 * 60 * 60 * 24 * 30 * 4},
            {"name": "6 months", "milliseconds": 1000 * 60 * 60 * 24 * 30 * 6},
            {"name": "1 year", "milliseconds": 1000 * 60 * 60 * 24 * 365 * 1},
        ]

        exact_ts_period = ts_range_ms / 40
        # Find the closest selection that's >= the exact period
        ts_period_ms = 0
        ts_period_name = None
        for selection in time_period_selections:
            if exact_ts_period <= selection["milliseconds"]:
                ts_period_ms = selection["milliseconds"]
                ts_period_name = selection["name"]
                break
        if ts_period_name is None:
            year_ms = 1000 * 60 * 60 * 24 * 365
            ts_period_years = math.ceil(exact_ts_period / year_ms)
            ts_period_ms = ts_period_years * year_ms
            ts_period_name = "{} years".format(ts_period_years)

        ts_graph_data = []
        ts_graph_data_pipeline = []
        if time_range["begin"] is not None:
            ts_graph_data_pipeline.append(
                {
                    "$match": {
                        "timestamp": {
                            "$gte": time_range["begin"],
                            "$lte": time_range["end"],
                        }
                    }
                }
            )
        ts_graph_data_pipeline.append(
            {
                "$group": {
                    "_id": {
                        "$subtract": [
                            "$timestamp",
                            {"$mod": ["$timestamp", ts_period_ms]},
                        ]
                    },
                    "count": {"$sum": 1},
                }
            }
        )
        ts_graph_data_pipeline.append({"$sort": {"_id": 1}})
        ts_graph_data_pipeline.append(
            {
                "$project": {
                    "_id": 1,
                    "count": 1,
                }
            }
        )
        last_ts = 0
        num_results = 0
        for doc in search_results_collection.aggregate(pipeline=ts_graph_data_pipeline):
            if len(ts_graph_data) > 0 and doc["_id"] > last_ts + ts_period_ms:
                ts_graph_data.append([last_ts + ts_period_ms, 0])

            ts_graph_data.append([doc["_id"], doc["count"]])
            num_results += doc["count"]

            last_ts = doc["_id"]
        # Finish last step of step graph
        ts_graph_data.append([last_ts + ts_period_ms, 0])

        # Update timeline graph
        doc = {
            "data": ts_graph_data,
            "num_results": num_results,
            "period_ms": ts_period_ms,
            "period_name": ts_period_name,
        }
        search_results_metadata_collection.update_one(
            {"_id": TIMELINE_DOC_ID}, {"$set": doc}, upsert=True
        )


def search_metadata_updater(
    db_uri: str,
    results_collection_name: str,
    results_metadata_collection_name: str,
    time_range,
    output_result_type: ResultType,
):
    # Connect to database
    client = MongoClient(db_uri)
    db = client.get_default_database()

    search_results_collection = pymongo.collection.Collection(
        db, results_collection_name
    )
    search_results_metadata_collection = pymongo.collection.Collection(
        db, results_metadata_collection_name
    )

    while True:
        update_timeline_and_count(
            search_results_collection,
            search_results_metadata_collection,
            time_range,
            output_result_type,
        )

        if g_query_done_event.is_set():
            break

        # Wait before next update
        time.sleep(1)


async def send_type_only_msg(websocket, msg_type: ServerMessageType):
    message = {"type": int(msg_type)}
    await websocket.send(json.dumps(message))


async def send_msg(websocket, msg_type: ServerMessageType, value):
    message = {"type": int(msg_type), "value": value}
    await websocket.send(json.dumps(message))


async def send_operation_complete_msg(websocket):
    await send_type_only_msg(websocket, ServerMessageType.OPERATION_COMPLETE)


async def send_preparing_for_query_msg(websocket):
    await send_type_only_msg(websocket, ServerMessageType.PREPARING_FOR_QUERY)


async def send_query_started_msg(websocket):
    await send_type_only_msg(websocket, ServerMessageType.QUERY_STARTED)


async def send_error_msg(websocket, msg):
    await send_msg(websocket, ServerMessageType.ERROR, msg)


def load_query_done_event(query_done_event: multiprocessing.Event):
    global g_query_done_event
    g_query_done_event = query_done_event


async def handle_unexpected_metadata_update_task_completion(websocket):
    logger.error("Search metadata update task finished while in unexpected state.")
    await send_error_msg(websocket, "query_handler: Internal error.")


async def handle_operation_task_completion(websocket, operation_task):
    try:
        return operation_task.result()
    except Exception as ex:
        await send_error_msg(websocket, str(ex))
        logger.exception("Operation failed.")
        return False

async def handle_receive_from_client_task_completion(websocket, task, pending_tasks):
    receive_successful, msg_from_client = task.result()
    if not receive_successful:
        return None, None
    receive_from_client_task: asyncio.Future = asyncio.ensure_future(
        receive_from_client(websocket)
    )
    pending_tasks.add(receive_from_client_task)
    return receive_from_client_task, msg_from_client


async def cancel_metadata_task(task, pending_tasks, query_done_event):
    task.cancel()
    try:
        await task
    except asyncio.CancelledError:
        pass
    if task in pending_tasks:
        pending_tasks.remove(task)
        query_done_event.clear()


def schedule_clear_results_task(
    db_uri: str,
    results_collection_name: str,
    results_metadata_collection_name: str,
    pending_tasks,
):
    operation_task = asyncio.ensure_future(
        run_function_in_process(
            clear_results,
            db_uri,
            results_collection_name,
            results_metadata_collection_name,
        )
    )
    pending_tasks.add(operation_task)
    return operation_task


async def cancel_operation_task(task, pending_tasks):
    task.cancel()
    try:
        await task
    except asyncio.CancelledError:
        pass
    if task in pending_tasks:
        pending_tasks.remove(task)


async def query_handler(
    websocket,
    request_uri: str,
    db_conn_conf: DatabaseConnectionConfig,
    results_cache_uri: str,
    results_collection_name: str,
    results_metadata_collection_name: str,
):
    logger.info(
        f"query_handler: Received connection from remote_address={websocket.remote_address} at request_uri={request_uri}"
    )

    _, sessionId = request_uri.split('/')
    logger.debug(f'query_handler: Received sessionId as {sessionId}')
    g_webui_connected[sessionId] = True

    session_results_collection_name = f"{results_collection_name}_{sessionId}"
    session_results_metadata_collection_name = f"{results_metadata_collection_name}_{sessionId}"

    pending = set()
    job_id = None
    operation_task: typing.Optional[asyncio.Future] = None
    metadata_update_task: typing.Optional[asyncio.Future] = None
    query_done_event = multiprocessing.Event()
    pending_query = None
    output_result_type = 0
    current_state = HandlerState.READY
    done = []

    try:
        # Add task to receive message from client
        receive_from_client_task: asyncio.Future = asyncio.ensure_future(
            receive_from_client(websocket)
        )
        pending.add(receive_from_client_task)

        while True:
            # Wait for pending tasks
            if len(done) == 0:
                logger.debug("Waiting for pending tasks...")
                done, pending = await asyncio.wait(
                    pending, return_when=asyncio.FIRST_COMPLETED
                )
                logger.debug(f"{len(done)} task(s) finished")

            if HandlerState.READY == current_state:
                if metadata_update_task in done:
                    done.remove(metadata_update_task)
                    query_done_event.clear()
                elif operation_task in done:
                    done.remove(operation_task)
                    logger.error("Operation task finished while in unexpected state.")
                    await send_error_msg(websocket, "query_handler: Internal error.")
                elif receive_from_client_task in done:
                    done.remove(receive_from_client_task)
                    (
                        receive_from_client_task,
                        msg_from_client,
                    ) = await handle_receive_from_client_task_completion(
                        websocket, receive_from_client_task, pending
                    )
                    if receive_from_client_task is None:
                        return

                    msg_type = msg_from_client["type"]
                    logger.info(msg_from_client)
                    if ClientMessageType.CANCEL_OPERATION == msg_type:
                        logger.debug("query_handler: CANCEL_OPERATION")
                        pass
                    elif ClientMessageType.CLEAR_RESULTS == msg_type:
                        if metadata_update_task is not None:
                            await cancel_metadata_task(
                                metadata_update_task, pending, query_done_event
                            )
                            metadata_update_task = None

                        operation_task = schedule_clear_results_task(
                            results_cache_uri,
                            session_results_collection_name,
                            session_results_metadata_collection_name,
                            pending,
                        )

                        current_state = HandlerState.CLEAR_RESULTS_IN_PROGRESS
                        logger.debug("READY -> CLEAR_RESULTS_IN_PROGRESS")
                    elif ClientMessageType.QUERY == msg_type:
                        pending_query = msg_from_client["query"]
                        logger.info(pending_query)
                        if not pending_query['pipeline_string']:
                            await send_error_msg(
                                websocket,
                                "query_handler: Internal error - Query cannot be empty.",
                            )
                            return

                        # Tell client we're preparing for the query
                        await send_preparing_for_query_msg(websocket)

                        if metadata_update_task is not None:
                            await cancel_metadata_task(
                                metadata_update_task, pending, query_done_event
                            )
                            metadata_update_task = None

                        operation_task = schedule_clear_results_task(
                            results_cache_uri,
                            session_results_collection_name,
                            session_results_metadata_collection_name,
                            pending,
                        )

                        current_state = (
                            HandlerState.CLEAR_RESULTS_IN_PROGRESS_BEFORE_QUERY
                        )
                        logger.debug("READY -> CLEAR_RESULTS_IN_PROGRESS_BEFORE_QUERY")
                    elif ClientMessageType.UPDATE_TIMELINE_RANGE == msg_type:
                        # Validate time range
                        try:
                            time_range = msg_from_client["time_range"]
                            if (
                                time_range["begin"] is not None
                                and time_range["end"] - time_range["begin"] <= 0
                            ):
                                await send_error_msg(
                                    websocket,
                                    "query_handler: Internal error - invalid time range.",
                                )
                                return
                        except KeyError:
                            await send_error_msg(
                                websocket,
                                "query_handler: Internal error - malformed time range message.",
                            )
                            return

                        if metadata_update_task is not None:
                            await cancel_metadata_task(
                                metadata_update_task, pending, query_done_event
                            )
                            metadata_update_task = None

                        # Schedule metadata update task
                        # One-shot update, so set query done now
                        query_done_event.set()
                        metadata_update_task = asyncio.ensure_future(
                            run_function_in_process(
                                search_metadata_updater,
                                results_cache_uri,
                                session_results_collection_name,
                                session_results_metadata_collection_name,
                                time_range,
                                output_result_type,
                                initializer=load_query_done_event,
                                init_args=(query_done_event,),
                            )
                        )
                        pending.add(metadata_update_task)
                else:
                    logger.error(done.pop())
            elif HandlerState.CLEAR_RESULTS_IN_PROGRESS == current_state:
                if metadata_update_task in done:
                    done.remove(metadata_update_task)
                    await handle_unexpected_metadata_update_task_completion(websocket)
                    return
                elif operation_task in done:
                    done.remove(operation_task)
                    await handle_operation_task_completion(websocket, operation_task)
                    operation_task = None

                    await send_operation_complete_msg(websocket)

                    current_state = HandlerState.READY
                    logger.debug("CLEAR_RESULTS_IN_PROGRESS -> READY")
                    continue
                elif receive_from_client_task in done:
                    done.remove(receive_from_client_task)
                    (
                        receive_from_client_task,
                        msg_from_client,
                    ) = await handle_receive_from_client_task_completion(
                        websocket, receive_from_client_task, pending
                    )
                    if receive_from_client_task is None:
                        return

                    msg_type = msg_from_client["type"]
                    if ClientMessageType.CANCEL_OPERATION == msg_type:
                        await cancel_operation_task(operation_task, pending)
                        operation_task = None
                        pending_query = None

                        current_state = HandlerState.READY
                        logger.debug("CLEAR_RESULTS_IN_PROGRESS -> READY")
                    elif ClientMessageType.CLEAR_RESULTS == msg_type:
                        await send_error_msg(
                            websocket,
                            "query_handler: Internal error - operation already in progress.",
                        )
                        return
                    elif ClientMessageType.QUERY == msg_type:
                        await send_error_msg(
                            websocket,
                            "query_handler: Internal error - operation already in progress.",
                        )
                        return
                    elif ClientMessageType.UPDATE_TIMELINE_RANGE == msg_type:
                        # Results are being cleared so ignore request
                        pass
            elif HandlerState.CLEAR_RESULTS_IN_PROGRESS_BEFORE_QUERY == current_state:
                if metadata_update_task in done:
                    done.remove(metadata_update_task)
                    await handle_unexpected_metadata_update_task_completion(websocket)
                    return
                elif operation_task in done:
                    done.remove(operation_task)
                    operation_succeeded = await handle_operation_task_completion(
                        websocket, operation_task
                    )
                    operation_task = None
                    if not operation_succeeded:
                        current_state = HandlerState.READY
                        logger.debug("CLEAR_RESULTS_IN_PROGRESS_BEFORE_QUERY -> READY")
                        continue

                    # Tell client we've started the query
                    await send_query_started_msg(websocket)

                    # Submit query synchronously so that we're guaranteed to get
                    # the job ID back
                    pending_query["results_collection_name"] = session_results_collection_name
                    job_id = submit_query(db_conn_conf, results_cache_uri,
                                          session_results_collection_name, pending_query)

                    operation_task = asyncio.ensure_future(
                        run_function_in_process(
                            wait_for_query_to_complete,
                            db_conn_conf,
                            job_id
                        )
                    )
                    pending.add(operation_task)

                    # Schedule metadata update task
                    output_result_type = ResultType.MESSAGE_RESULT

                    metadata_update_task = asyncio.ensure_future(
                        run_function_in_process(
                            search_metadata_updater,
                            results_cache_uri,
                            session_results_collection_name,
                            session_results_metadata_collection_name,
                            {"begin": None, "end": None},
                            output_result_type,
                            initializer=load_query_done_event,
                            init_args=(query_done_event,),
                        )
                    )
                    pending.add(metadata_update_task)
                    pending_query = None

                    current_state = HandlerState.QUERY_IN_PROGRESS
                    logger.debug(
                        "CLEAR_RESULTS_IN_PROGRESS_BEFORE_QUERY -> QUERY_IN_PROGRESS"
                    )
                    continue
                elif receive_from_client_task in done:
                    done.remove(receive_from_client_task)
                    (
                        receive_from_client_task,
                        msg_from_client,
                    ) = await handle_receive_from_client_task_completion(
                        websocket, receive_from_client_task, pending
                    )
                    if receive_from_client_task is None:
                        return

                    msg_type = msg_from_client["type"]
                    if ClientMessageType.CANCEL_OPERATION == msg_type:
                        await cancel_operation_task(operation_task, pending)
                        operation_task = None
                        pending_query = None

                        current_state = HandlerState.READY
                        logger.debug("CLEAR_RESULTS_IN_PROGRESS_BEFORE_QUERY -> READY")
                    elif ClientMessageType.CLEAR_RESULTS == msg_type:
                        await send_error_msg(
                            websocket,
                            "query_handler: Internal error - operation already in progress.",
                        )
                        return
                    elif ClientMessageType.QUERY == msg_type:
                        await send_error_msg(
                            websocket,
                            "query_handler: Internal error - operation already in progress.",
                        )
                        return
                    elif ClientMessageType.UPDATE_TIMELINE_RANGE == msg_type:
                        # Results are being cleared so ignore request
                        pass
            elif HandlerState.QUERY_IN_PROGRESS == current_state:
                if metadata_update_task in done:
                    done.remove(metadata_update_task)
                    await handle_unexpected_metadata_update_task_completion(websocket)
                    return
                elif operation_task in done:
                    done.remove(operation_task)
                    await handle_operation_task_completion(websocket, operation_task)
                    operation_task = None
                    job_id = None

                    await send_operation_complete_msg(websocket)

                    current_state = HandlerState.READY
                    logger.debug("QUERY_IN_PROGRESS -> READY")
                    continue
                elif receive_from_client_task in done:
                    done.remove(receive_from_client_task)
                    (
                        receive_from_client_task,
                        msg_from_client,
                    ) = await handle_receive_from_client_task_completion(
                        websocket, receive_from_client_task, pending
                    )
                    if receive_from_client_task is None:
                        return

                    msg_type = msg_from_client["type"]
                    if ClientMessageType.CANCEL_OPERATION == msg_type:
                        await cancel_operation_task(operation_task, pending)
                        operation_task = None
                        pending_query = None

                        cancel_query(db_conn_conf, job_id)
                        job_id = None

                        # Signal metadata updater to stop
                        query_done_event.set()

                        current_state = HandlerState.READY
                        logger.debug("QUERY_IN_PROGRESS -> READY")
                    elif ClientMessageType.CLEAR_RESULTS == msg_type:
                        await send_error_msg(
                            websocket,
                            "query_handler: Internal error - operation already in progress.",
                        )
                        return
                    elif ClientMessageType.QUERY == msg_type:
                        await send_error_msg(
                            websocket,
                            "query_handler: Internal error - operation already in progress.",
                        )
                        return
                    elif ClientMessageType.UPDATE_TIMELINE_RANGE == msg_type:
                        # Validate time range
                        try:
                            time_range = msg_from_client["time_range"]
                            if (
                                time_range["begin"] is not None
                                and time_range["end"] - time_range["begin"] <= 0
                            ):
                                await send_error_msg(
                                    websocket,
                                    "query_handler: Internal error - invalid time range.",
                                )
                                return
                        except KeyError:
                            await send_error_msg(
                                websocket,
                                "query_handler: Internal error - malformed time range message.",
                            )
                            return

                        if metadata_update_task is not None:
                            await cancel_metadata_task(
                                metadata_update_task, pending, query_done_event
                            )
                            metadata_update_task = None

                        # Schedule metadata update task
                        metadata_update_task = asyncio.ensure_future(
                            run_function_in_process(
                                search_metadata_updater,
                                results_cache_uri,
                                session_results_collection_name,
                                session_results_metadata_collection_name,
                                time_range,
                                output_result_type,
                                initializer=load_query_done_event,
                                init_args=(query_done_event,),
                            )
                        )
                        pending.add(metadata_update_task)
            else:
                raise NotImplementedError()
    except Exception:
        logger.exception("query_handler: Unexpected exception.")
    finally:
        # Cancel query if running
        if job_id is not None:
            cancel_query(db_conn_conf, job_id)

        # Cancel tasks
        cancellation_tasks = set()
        for task in pending:
            task.cancel()
            cancellation_tasks.add(task)
        if len(cancellation_tasks) > 0:
            await asyncio.wait(cancellation_tasks, return_when=asyncio.ALL_COMPLETED)

        await websocket.close()

        g_webui_connected[sessionId] = False


def main(argv):
    args_parser = argparse.ArgumentParser(
        description="Query handler for queries from the WebUI."
    )
    args_parser.add_argument(
        "--host", required=True, help="Host to bind the websocket to."
    )
    args_parser.add_argument(
        "--port", required=True, type=int, help="Port to bind the websocket to."
    )
    args_parser.add_argument(
        "--db-host", required=True, help="Search jobs database host."
    )
    args_parser.add_argument(
        "--db-port", required=True, type=int, help="Search jobs database port."
    )
    args_parser.add_argument(
        "--db-name", required=True, help="Search jobs database name."
    )
    args_parser.add_argument(
        "--results-cache-uri", required=True, help="Results cache URI."
    )
    args_parser.add_argument(
        "--results-cache-results-collection",
        required=True,
        help="Collection for results.",
    )
    args_parser.add_argument(
        "--results-cache-metadata-collection",
        required=True,
        help="Collection for results metadata.",
    )

    parsed_args = args_parser.parse_args(argv[1:])
    websocket_host: str = parsed_args.host
    websocket_port: int = parsed_args.port
    db_host: str = parsed_args.db_host
    db_port: int = parsed_args.db_port
    db_name: str = parsed_args.db_name
    results_cache_uri = parsed_args.results_cache_uri
    results_collection_name = parsed_args.results_cache_results_collection
    results_metadata_collection_name = parsed_args.results_cache_metadata_collection

    # Setup logging to file
    log_file = Path(os.getenv("CLP_LOGS_DIR")) / "webui_query_handler.log"
    logging_file_handler = logging.FileHandler(filename=log_file, encoding="utf-8")
    logging_file_handler.setFormatter(logging_formatter)
    logger.addHandler(logging_file_handler)

    # Update logging level based on config
    logging_level_str = os.getenv("CLP_LOGGING_LEVEL")
    logging_level = get_logging_level(logging_level_str)
    logger.setLevel(logging_level)
    logger.info(f"Start logging level = {logging.getLevelName(logging_level)}")

    try:
        # TODO Extract constant names into variables
        db_user = os.environ["CLP_DB_USER"]
        db_pass = os.environ["CLP_DB_PASS"]
    except KeyError as e:
        logger.exception(f"Database credentials ({e}) not specified.")
        return -1

    db_conn_conf = DatabaseConnectionConfig(
        host=db_host, port=db_port, db_name=db_name, user=db_user, password=db_pass
    )

    query_handler_with_args = functools.partial(
        query_handler,
        db_conn_conf=db_conn_conf,
        results_cache_uri=results_cache_uri,
        results_collection_name=results_collection_name,
        results_metadata_collection_name=results_metadata_collection_name,
    )
    loop = asyncio.get_event_loop()
    server_task = websockets.serve(
        query_handler_with_args, host=websocket_host, port=websocket_port, loop=loop
    )
    server = loop.run_until_complete(server_task)

    logger.info("webui-query-handler started successfully.")

    try:
        loop.run_forever()
    except KeyboardInterrupt:
        pass

    server.close()
    loop.run_until_complete(server.wait_closed())
    loop.close()

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
