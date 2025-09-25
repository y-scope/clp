#!/usr/bin/env python3

import argparse
import asyncio
import os
import sys
from pathlib import Path
from typing import Callable, Dict, List, Optional, Tuple

from clp_py_utils.clp_config import (
    CLPConfig,
    GARBAGE_COLLECTOR_COMPONENT_NAME,
)
from clp_py_utils.clp_logging import get_logger
from clp_py_utils.core import read_yaml_config_file
from job_orchestration.garbage_collector.archive_garbage_collector import archive_garbage_collector
from job_orchestration.garbage_collector.constants import (
    ARCHIVE_GARBAGE_COLLECTOR_NAME,
    SEARCH_RESULT_GARBAGE_COLLECTOR_NAME,
)
from job_orchestration.garbage_collector.search_result_garbage_collector import (
    search_result_garbage_collector,
)
from job_orchestration.garbage_collector.utils import configure_logger
from pydantic import ValidationError

logger = get_logger(GARBAGE_COLLECTOR_COMPONENT_NAME)


async def main(argv: List[str]) -> int:
    args_parser = argparse.ArgumentParser(
        description=f"Spin up the {GARBAGE_COLLECTOR_COMPONENT_NAME}."
    )
    args_parser.add_argument("--config", "-c", required=True, help="CLP configuration file.")
    parsed_args = args_parser.parse_args(argv[1:])

    # Setup logging to file
    logs_directory = Path(os.getenv("CLP_LOGS_DIR"))
    logging_level = os.getenv("CLP_LOGGING_LEVEL")
    configure_logger(logger, logging_level, logs_directory, GARBAGE_COLLECTOR_COMPONENT_NAME)

    # Load configuration
    config_path = Path(parsed_args.config)
    try:
        clp_config = CLPConfig.model_validate(read_yaml_config_file(config_path))
        clp_config.database.load_credentials_from_env()
    except (ValidationError, ValueError) as err:
        logger.error(err)
        return 1
    except Exception:
        logger.exception("Failed to parse CLP configuration file.")
        return 1

    gc_task_configs: Dict[str, Tuple[Optional[int], Callable]] = {
        ARCHIVE_GARBAGE_COLLECTOR_NAME: (
            clp_config.archive_output.retention_period,
            archive_garbage_collector,
        ),
        SEARCH_RESULT_GARBAGE_COLLECTOR_NAME: (
            clp_config.results_cache.retention_period,
            search_result_garbage_collector,
        ),
    }
    gc_tasks: List[asyncio.Task[None]] = []

    # Create GC tasks
    for gc_name, (retention_period, task_method) in gc_task_configs.items():
        if retention_period is None:
            logger.info(f"Retention period is not configured, skip creating {gc_name}.")
            continue
        logger.info(f"Creating {gc_name} with retention period = {retention_period} minutes")
        gc_tasks.append(
            asyncio.create_task(
                task_method(clp_config, logs_directory, logging_level), name=gc_name
            )
        )

    # Poll and report any task that finished unexpectedly
    while len(gc_tasks) != 0:
        done, _ = await asyncio.wait(gc_tasks, return_when=asyncio.FIRST_COMPLETED)
        for task in done:
            gc_tasks.remove(task)
            gc_name = task.get_name()
            try:
                _ = task.result()
                logger.error(f"{gc_name} unexpectedly terminated without an error.")
            except Exception as e:
                logger.exception(f"{gc_name} failed.")

    logger.error("All garbage collectors terminated unexpectedly.")
    return 1


if "__main__" == __name__:
    sys.exit(asyncio.run(main(sys.argv)))
