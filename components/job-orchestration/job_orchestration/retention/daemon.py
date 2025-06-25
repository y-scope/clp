#!/usr/bin/env python3

import argparse
import asyncio
import logging
import os
import sys
from pathlib import Path
from typing import List

from clp_py_utils.clp_config import (
    CLPConfig,
    RETENTION_DAEMON_COMPONENT_NAME,
)
from clp_py_utils.clp_logging import get_logger, get_logging_formatter, set_logging_level
from clp_py_utils.core import read_yaml_config_file
from job_orchestration.retention.archives_handler import archive_retention
from job_orchestration.retention.search_results_handler import search_results_retention
from job_orchestration.retention.streams_handler import stream_retention
from pydantic import ValidationError

logger = get_logger(RETENTION_DAEMON_COMPONENT_NAME)


async def main(argv: List[str]) -> int:

    args_parser = argparse.ArgumentParser(
        description=f"Spin up the {RETENTION_DAEMON_COMPONENT_NAME}."
    )
    args_parser.add_argument("--config", "-c", required=True, help="CLP configuration file.")

    parsed_args = args_parser.parse_args(argv[1:])

    # Setup logging to file
    logs_directory = Path(os.getenv("CLP_LOGS_DIR"))
    logging_level = os.getenv("CLP_LOGGING_LEVEL")
    log_file = logs_directory / f"{RETENTION_DAEMON_COMPONENT_NAME}.log"
    logging_file_handler = logging.FileHandler(filename=log_file, encoding="utf-8")
    logging_file_handler.setFormatter(get_logging_formatter())
    logger.addHandler(logging_file_handler)

    # Update logging level based on config
    set_logging_level(logger, logging_level)

    # Load configuration
    config_path = Path(parsed_args.config)
    try:
        clp_config = CLPConfig.parse_obj(read_yaml_config_file(config_path))
    except ValidationError as err:
        logger.error(err)
        return 1
    except Exception as ex:
        logger.error(ex)
        return 1

    retention_tasks = []

    archive_retention_handle = asyncio.create_task(
        archive_retention(clp_config, logs_directory, logging_level), name="archive_retention"
    )
    retention_tasks.append(archive_retention_handle)

    streams_retention_handle = asyncio.create_task(
        stream_retention(clp_config, logs_directory, logging_level), name="stream_retention"
    )
    retention_tasks.append(streams_retention_handle)

    search_results_retention_handle = asyncio.create_task(
        search_results_retention(clp_config, logs_directory, logging_level),
        name="search_results_retention",
    )
    retention_tasks.append(search_results_retention_handle)

    while retention_tasks:
        done, pending = await asyncio.wait(retention_tasks, return_when=asyncio.FIRST_COMPLETED)

        for task in done:
            retention_tasks.remove(task)
            task_name = task.get_name()
            try:
                _ = task.result()
                logger.info(f"Task {task_name} terminated without error.")
            except Exception as e:
                logger.error(f"Task {task_name} failed with error: {e}", exc_info=True)

    logger.info(f"No retention tasks are running, daemon terminates.")
    return 0


if "__main__" == __name__:
    sys.exit(asyncio.run(main(sys.argv)))
