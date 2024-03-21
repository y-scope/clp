import asyncio
import enum
from enum import Enum
from typing import Any


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
