#!/usr/bin/env python3

import argparse
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
from job_orchestration.retention.archives_handler import archive_retention_entry
from pydantic import ValidationError

logger = get_logger(RETENTION_DAEMON_COMPONENT_NAME)


def main(argv: List[str]) -> int:
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

    # fmt: off
    archive_retention_period = clp_config.archive_output.retention_period
    logger.info(f"Archive retention period: {archive_retention_period}")
    stream_retention_period = clp_config.stream_output.retention_period
    logger.info(f"Stream retention period: {stream_retention_period}")
    results_cache_retention_period = clp_config.results_cache.retention_period
    logger.info(f"Results cache retention period: {results_cache_retention_period}")

    archive_retention_entry(clp_config, 30, logs_directory, logging_level)

    logger.info("reducer terminated")

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
