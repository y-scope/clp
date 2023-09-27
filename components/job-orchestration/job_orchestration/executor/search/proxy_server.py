#!/usr/bin/env python3
import argparse
import asyncio
import datetime
import logging
import multiprocessing
import os
import pathlib
import socket
import sys
import time
from asyncio import StreamReader, StreamWriter
from contextlib import closing
import pymongo

import functools
import msgpack
logger = logging.getLogger(__file__)


def make_mongo_record(packed_message):
    return {"file" : packed_message[0], "message" : packed_message[2], "timestamp" : packed_message[1]}


def process_batched_messages(batched_messages, db_config, job_id):
    if len(batched_messages) == 0:
        return
    with pymongo.MongoClient(db_config["uri"]) as db_client:
        mydb = db_client[db_config["name"]]
        mycol = mydb[job_id]
        append_list = []
        for unpacked in batched_messages:
            append_list.append(make_mongo_record(unpacked))
        x = mycol.insert_many(append_list)

async def worker_connection_handler(reader: StreamReader, writer: StreamWriter, db_config, job_id):
    try:
        unpacker = msgpack.Unpacker()
        unpacked_messages = []
        while True:
            # Read some data from the worker and feed it to msgpack
            buf = await reader.read(1024)
            if not buf:
                # Worker closed
                process_batched_messages(unpacked_messages, db_config, job_id)
                unpacked_messages.clear()
                # This is very hacky. but hopefully it's ok for now and
                # will be replaced with reducer.
                print("Finished")
                sys.stdout.flush()
                return
            unpacker.feed(buf)

            # Print out any messages we can decode
            for unpacked in unpacker:
                unpacked_messages.append(unpacked)

            if len(unpacked_messages) >= 1000:
                process_batched_messages(unpacked_messages, db_config, job_id)
                unpacked_messages.clear()

    except asyncio.CancelledError:
        return
    finally:
        writer.close()


async def do_search(host: str, db_config, job_id):
    # Start server to receive and print results
    try:
        server = await asyncio.start_server(functools.partial(worker_connection_handler, db_config=db_config, job_id = job_id),
                                            host=host, port=0, family=socket.AF_INET)
    except asyncio.CancelledError:
        # Search cancelled
        return
    port = server.sockets[0].getsockname()[1]
    print(host, port)
    sys.stdout.flush()
    server_task = asyncio.ensure_future(server.serve_forever())

    try:
        done = await server_task
    except asyncio.CancelledError:
        server.close()
        await server.wait_closed()


def main(argv):
    args_parser = argparse.ArgumentParser(description="Proxy server")
    args_parser.add_argument(
        "db_host", help="host ip of the mongoDB"
    )
    args_parser.add_argument(
        "db_port", help="port of the mongoDB"
    )
    args_parser.add_argument(
        "db_name", help="db_name"
    )
    args_parser.add_argument(
        "job_id", help="job_id"
    )
    parsed_args = args_parser.parse_args(argv[1:])


    # Get IP of local machine
    host_ip = None
    for ip in set(socket.gethostbyname_ex(socket.gethostname())[2]):
        host_ip = ip
        break
    if host_ip is None:
        logger.error("Could not determine IP of local machine.")
        return -1

    db_config = {
        "uri" : f"mongodb://{parsed_args.db_host}:{parsed_args.db_port}/",
        "name" : parsed_args.db_name
    }

    asyncio.run(do_search(host_ip, db_config, parsed_args.job_id))

    return 0


if '__main__' == __name__:
    sys.exit(main(sys.argv))