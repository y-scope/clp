#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys
from pathlib import Path
from typing import TextIO

from clp_py_utils.clp_config import ClpConfig
from clp_py_utils.clp_logging import configure_logging, get_logger
from clp_py_utils.core import read_yaml_config_file
from pydantic import ValidationError

logger = get_logger("reducer")


def main(argv: list[str]) -> int:
    args_parser = argparse.ArgumentParser(description="Spin up reducers.")
    args_parser.add_argument("--config", "-c", required=True, help="CLP configuration file.")
    args_parser.add_argument(
        "--concurrency", required=True, help="Number of reducer servers to run"
    )
    args_parser.add_argument(
        "--upsert-interval",
        required=True,
        help="Interval for upserting timeline aggregation results (ms)",
    )

    parsed_args = args_parser.parse_args(argv[1:])

    # Setup optional file logging and logging level.
    configure_logging(logger, "reducer")

    # Load configuration
    config_path = Path(parsed_args.config)
    try:
        clp_config = ClpConfig.model_validate(read_yaml_config_file(config_path))
    except ValidationError as err:
        logger.error(err)
        return 1
    except Exception as ex:
        logger.error(ex)
        return 1

    clp_home = Path(os.getenv("CLP_HOME"))
    # fmt: off
    reducer_cmd = [
        str(clp_home / "bin" / "reducer-server"),
        "--scheduler-host", clp_config.query_scheduler.host,
        "--scheduler-port", str(clp_config.query_scheduler.port),
        "--mongodb-uri", clp_config.results_cache.get_uri(),
        "--upsert-interval", str(parsed_args.upsert_interval),
        "--reducer-host", clp_config.reducer.host,
        "--reducer-port",
    ]
    # fmt: on

    logs_dir_str = os.getenv("CLP_LOGS_DIR")
    logs_dir = Path(logs_dir_str) if logs_dir_str else None

    reducers = []
    reducer_log_files: list[TextIO] = []
    concurrency = max(int(parsed_args.concurrency), 1)
    for i in range(concurrency):
        reducer_instance_cmd = reducer_cmd + [str(clp_config.reducer.base_port + i)]

        reducer_stdout = sys.stdout
        reducer_stderr = sys.stderr
        if logs_dir is not None:
            log_file_path = logs_dir / f"reducer-{i}.log"
            log_file = open(log_file_path, "a")
            reducer_log_files.append(log_file)
            reducer_stdout = log_file
            reducer_stderr = log_file

        reducers.append(
            subprocess.Popen(
                reducer_instance_cmd,
                close_fds=True,
                stdout=reducer_stdout,
                stderr=reducer_stderr,
            )
        )

    logger.info("Reducers started.")
    logger.info(
        f"Host={clp_config.reducer.host}"
        f" Base port={clp_config.reducer.base_port}"
        f" Concurrency={concurrency}"
        f" Upsert Interval={parsed_args.upsert_interval}"
    )
    for i, reducer in enumerate(reducers):
        reducer.communicate()
        logger.info(f"reducer-{i} exited with returncode={reducer.returncode}")
    for reducer_log_file in reducer_log_files:
        reducer_log_file.close()

    logger.error("All reducers terminated")

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
