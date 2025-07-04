#!/usr/bin/env python3

import argparse
import asyncio
import os
import sys
from pathlib import Path
from typing import Callable, Dict, List, Optional, Tuple

from clp_py_utils.clp_config import (
    CLPConfig,
    RETENTION_CLEANER_COMPONENT_NAME,
)
from clp_py_utils.clp_logging import get_logger
from clp_py_utils.core import read_yaml_config_file
from job_orchestration.retention.archives_handler import archive_retention
from job_orchestration.retention.constants import (
    ARCHIVES_RETENTION_HANDLER_NAME,
    SEARCH_RESULTS_RETENTION_HANDLER_NAME,
)
from job_orchestration.retention.search_results_handler import search_results_retention
from job_orchestration.retention.utils import configure_logger
from pydantic import ValidationError

logger = get_logger(RETENTION_CLEANER_COMPONENT_NAME)


async def main(argv: List[str]) -> int:
    args_parser = argparse.ArgumentParser(
        description=f"Spin up the {RETENTION_CLEANER_COMPONENT_NAME}."
    )
    args_parser.add_argument("--config", "-c", required=True, help="CLP configuration file.")
    parsed_args = args_parser.parse_args(argv[1:])

    # Setup logging to file
    logs_directory = Path(os.getenv("CLP_LOGS_DIR"))
    logging_level = os.getenv("CLP_LOGGING_LEVEL")
    configure_logger(logger, logging_level, logs_directory, RETENTION_CLEANER_COMPONENT_NAME)

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

    retention_tasks: Dict[str, Tuple[Optional[int], Callable]] = {
        ARCHIVES_RETENTION_HANDLER_NAME: (
            clp_config.archive_output.retention_period,
            archive_retention,
        ),
        SEARCH_RESULTS_RETENTION_HANDLER_NAME: (
            clp_config.results_cache.retention_period,
            search_results_retention,
        ),
    }
    tasks_handler: List[asyncio.Task[None]] = []

    # Create retention tasks
    for task_name, (retention_period, task_method) in retention_tasks.items():
        if retention_period is not None:
            logger.info(f"Creating {task_name} task with retention = {retention_period} minutes")
            retention_task = asyncio.create_task(
                task_method(clp_config, logs_directory, logging_level), name=task_name
            )
            tasks_handler.append(retention_task)
        else:
            logger.info(f"No retention period configured, skip creating {task_name} task.")

    # Poll and report any task that finishes unexpectedly
    while tasks_handler:
        done, _ = await asyncio.wait(tasks_handler, return_when=asyncio.FIRST_COMPLETED)
        for task in done:
            tasks_handler.remove(task)
            task_name = task.get_name()
            try:
                _ = task.result()
                logger.info(f"Task {task_name} unexpectedly terminated without an error.")
            except Exception as e:
                logger.exception(f"Task {task_name} failed with exception: {e}")

    logger.error("All retention tasks unexpectedly terminated.")
    return -1


if "__main__" == __name__:
    sys.exit(asyncio.run(main(sys.argv)))
