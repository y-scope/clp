import asyncio
import enum
from enum import Enum
from typing import Any, Optional

import msgpack
from clp_py_utils.clp_logging import get_logger
from job_orchestration.scheduler.job_config import AggregationConfig

# Setup logging
logger = get_logger("reducer_handler")


class ReducerHandlerMessageType(Enum):
    SUCCESS = enum.auto()
    FAILURE = enum.auto()
    AGGREGATION_CONFIG = enum.auto()


class ReducerHandlerMessage:
    def __init__(self, msg_type: ReducerHandlerMessageType, payload: Any = None):
        self.msg_type = msg_type
        self.payload = payload


class ReducerHandlerMessageQueues:
    """
    Class to encapsulate the incoming and outgoing message queues for the reducer handler.
    """

    def __init__(self):
        self.__incoming_msg_queue = asyncio.Queue()
        self.__outgoing_msg_queue = asyncio.Queue()

    async def put_to_handler(self, msg: ReducerHandlerMessage):
        await self.__incoming_msg_queue.put(msg)

    async def get_from_handler(self) -> ReducerHandlerMessage:
        return await self.__outgoing_msg_queue.get()

    async def put_to_listeners(self, msg: ReducerHandlerMessage):
        await self.__outgoing_msg_queue.put(msg)

    async def get_from_listeners(self) -> ReducerHandlerMessage:
        return await self.__incoming_msg_queue.get()


class _ReducerHandlerWaitState(Enum):
    """
    An enum representing the different states the handler can be in. Each state is named
    according to the item the handler is waiting for.
    """

    JOB_CONFIG = enum.auto()
    JOB_CONFIG_ACK = enum.auto()
    QUERY_WORKERS_DONE = enum.auto()
    REDUCER_DONE = enum.auto()


async def _handle_unexpected_msg_from_reducer(
    state: _ReducerHandlerWaitState, msg_queues: ReducerHandlerMessageQueues
):
    logger.error(f"[_ReducerHandleWaitState.{state.name}] Unexpected message from reducer.")
    msg = ReducerHandlerMessage(ReducerHandlerMessageType.FAILURE)
    await msg_queues.put_to_listeners(msg)


async def _handle_unexpected_msg_from_listener(
    state: _ReducerHandlerWaitState,
    msg_type: ReducerHandlerMessageType,
    msg_queues: ReducerHandlerMessageQueues,
):
    logger.error(
        f"[_ReducerHandleWaitState.{state.name}] Unexpected message type {msg_type.name} from"
        f" listener."
    )
    msg = ReducerHandlerMessage(ReducerHandlerMessageType.FAILURE)
    await msg_queues.put_to_listeners(msg)


async def _recv_msg_from_reducer(reader: asyncio.StreamReader) -> bytes:
    """
    Receives and deserializes a message from the connected reducer
    :param reader: StreamReader connected to a reducer
    :return: The received message
    """
    msg_size_bytes = await reader.readexactly(8)
    msg_size = int.from_bytes(msg_size_bytes, byteorder="little")
    return await reader.readexactly(msg_size)


async def _send_msg_to_reducer(msg: bytes, writer: asyncio.StreamWriter):
    """
    Serializes and sends a message to the connected reducer
    :param msg:
    :param writer: StreamWriter connected to a reducer
    """
    msg_size_bytes = (len(msg)).to_bytes(8, byteorder="little")
    writer.write(msg_size_bytes)
    writer.write(msg)
    await writer.drain()


async def handle_reducer_connection(
    reader: asyncio.StreamReader,
    writer: asyncio.StreamWriter,
    reducer_connection_queue: asyncio.Queue,
):
    try:
        message_bytes = await _recv_msg_from_reducer(reader)
        reducer_addr_info = msgpack.unpackb(message_bytes)

        msg_queues = ReducerHandlerMessageQueues()
        await reducer_connection_queue.put(
            (reducer_addr_info["host"], reducer_addr_info["port"], msg_queues)
        )

        """
        In the state handling loop below, the logic only expects one of the two tasks to finish when
        in a given state. This allows us to write the logic for each case in the form of:
        if unexpected_task in done:
            # Handle unexpected_task
            return

        # Handle expected_task
        # Reschedule expected_task
        # Transition to next state
        """
        current_wait_state: _ReducerHandlerWaitState = _ReducerHandlerWaitState.JOB_CONFIG
        recv_listener_msg_task: Optional[asyncio.Task] = asyncio.create_task(
            msg_queues.get_from_listeners()
        )
        recv_reducer_msg_task: Optional[asyncio.Task] = asyncio.create_task(reader.readexactly(1))
        while True:
            pending = [recv_listener_msg_task, recv_reducer_msg_task]
            done, pending = await asyncio.wait(pending, return_when=asyncio.FIRST_COMPLETED)

            if _ReducerHandlerWaitState.JOB_CONFIG == current_wait_state:
                if recv_reducer_msg_task in done:
                    await _handle_unexpected_msg_from_reducer(current_wait_state, msg_queues)
                    return

                # Get the aggregation config from the listener and send the necessary info to the
                # reducer
                msg: ReducerHandlerMessage = recv_listener_msg_task.result()
                if ReducerHandlerMessageType.AGGREGATION_CONFIG != msg.msg_type:
                    await _handle_unexpected_msg_from_listener(
                        current_wait_state, msg.msg_type, msg_queues
                    )
                aggregation_config: AggregationConfig = msg.payload
                job_id = aggregation_config.job_id
                time_bucket_size = aggregation_config.count_by_time_bucket_size
                await _send_msg_to_reducer(
                    msgpack.packb(
                        {
                            "job_id": job_id,
                            "count_by_time_bucket_size": time_bucket_size,
                        }
                    ),
                    writer,
                )

                recv_listener_msg_task = asyncio.create_task(msg_queues.get_from_listeners())
                current_wait_state = _ReducerHandlerWaitState.JOB_CONFIG_ACK
            elif _ReducerHandlerWaitState.JOB_CONFIG_ACK == current_wait_state:
                if recv_listener_msg_task in done:
                    msg: ReducerHandlerMessage = recv_listener_msg_task.result()
                    if ReducerHandlerMessageType.FAILURE == msg.msg_type:
                        # Listener requested cancellation
                        return
                    else:
                        await _handle_unexpected_msg_from_listener(
                            current_wait_state, msg.msg_type, msg_queues
                        )
                        return

                # Tell the listener the reducer ACKed the job
                msg = ReducerHandlerMessage(ReducerHandlerMessageType.SUCCESS)
                await msg_queues.put_to_listeners(msg)

                recv_reducer_msg_task = asyncio.create_task(reader.readexactly(1))
                current_wait_state = _ReducerHandlerWaitState.QUERY_WORKERS_DONE
            elif _ReducerHandlerWaitState.QUERY_WORKERS_DONE == current_wait_state:
                if recv_reducer_msg_task in done:
                    await _handle_unexpected_msg_from_reducer(current_wait_state, msg_queues)
                    return

                msg: ReducerHandlerMessage = recv_listener_msg_task.result()
                if ReducerHandlerMessageType.FAILURE == msg.msg_type:
                    # Listener requested cancellation
                    return
                elif ReducerHandlerMessageType.SUCCESS != msg.msg_type:
                    await _handle_unexpected_msg_from_listener(
                        current_wait_state, msg.msg_type, msg_queues
                    )
                    return

                # Tell the reducer the query workers are done
                await _send_msg_to_reducer(msgpack.packb({"done": True}), writer)

                recv_listener_msg_task = asyncio.create_task(msg_queues.get_from_listeners())
                current_wait_state = _ReducerHandlerWaitState.REDUCER_DONE
            elif _ReducerHandlerWaitState.REDUCER_DONE == current_wait_state:
                if recv_listener_msg_task in done:
                    msg: ReducerHandlerMessage = recv_listener_msg_task.result()
                    if ReducerHandlerMessageType.FAILURE == msg.msg_type:
                        # Listener requested cancellation
                        return
                    else:
                        await _handle_unexpected_msg_from_listener(
                            current_wait_state, msg.msg_type, msg_queues
                        )
                        return

                msg = ReducerHandlerMessage(ReducerHandlerMessageType.SUCCESS)
                await msg_queues.put_to_listeners(msg)
                break
    finally:
        writer.close()
        await writer.wait_closed()
