#!/usr/bin/env python3

import argparse
import logging
import os
import subprocess
import sys
from pathlib import Path
from typing import List

from clp_py_utils.clp_config import CLPConfig
from clp_py_utils.clp_logging import get_logger, get_logging_formatter, set_logging_level
from clp_py_utils.core import read_yaml_config_file
from pydantic import ValidationError

# Setup logging
logger = get_logger("reducer")


def main(argv: List[str]) -> int:
    # fmt: off
    args_parser = argparse.ArgumentParser(description="Spin up reducers.")
    args_parser.add_argument('--config', '-c', required=True, help='CLP configuration file.')
    args_parser.add_argument('--host', required=True, help='Host ip this container is running on')
    args_parser.add_argument('--concurrency', required=True, help='Number of reducer servers to run')
    args_parser.add_argument('--polling-interval-ms', required=True, help='Database polling interval in ms')

    parsed_args = args_parser.parse_args(argv[1:])

    # Setup logging to file
    logs_dir = Path(os.getenv("CLP_LOGS_DIR"))
    log_file = Path(os.getenv("CLP_LOGS_DIR")) / "reducer.log"
    logging_file_handler = logging.FileHandler(filename=log_file, encoding="utf-8")
    logging_file_handler.setFormatter(get_logging_formatter())
    logger.addHandler(logging_file_handler)

    # Update logging level based on config
    logging_level_str = os.getenv("CLP_LOGGING_LEVEL")
    set_logging_level(logger, logging_level_str)

    # Load configuration
    config_path = Path(parsed_args.config)
    try:
        clp_config = CLPConfig.parse_obj(read_yaml_config_file(config_path))
    except ValidationError as err:
        logger.error(err)
        return -1
    except Exception as ex:
        logger.error(ex)
        # read_yaml_config_file already logs the parsing error inside
        return -1

    clp_home = Path(os.getenv("CLP_HOME"))
    reducer_cmd = [
        str(clp_home / "bin" / "reducer-server"),
        "--scheduler-host", clp_config.search_scheduler.host,
        "--scheduler-port", str(clp_config.search_scheduler.port),
        "--mongodb-uri", clp_config.results_cache.get_uri(),
        "--polling-interval-ms", str(parsed_args.polling_interval_ms),
        "--reducer-host", parsed_args.host,
        "--reducer-port",
    ]

    reducers = []
    concurrency = max(int(parsed_args.concurrency), 1)
    for i in range(concurrency):
        reducer_instance_cmd = reducer_cmd + [str(clp_config.reducer.base_port + i)]

        log_file_path = logs_dir / ("reducer-" + str(i) + ".log")
        log_file = open(log_file_path, 'a')

        reducers.append(
            subprocess.Popen(
                reducer_instance_cmd,
                close_fds=True,
                stdout=log_file,
                stderr=log_file,
            )
        )

    logger.info("reducers started.")
    logger.info(f"Host={parsed_args.host} Base port={clp_config.reducer.base_port} Concurrency={concurrency} Polling Interval={parsed_args.polling_interval_ms}")
    for i in range(len(reducers)):
        reducers[i].communicate()
        logger.info(f"reducer-{i} exited with returncode={reducers[i].returncode}")

    logger.error("all reducers terminated")

    logger.removeHandler(logging_file_handler)
    logging_file_handler.close()

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
