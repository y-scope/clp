import argparse
import logging
import os
import pathlib
import subprocess
import sys
import uuid

import yaml
from clp_py_utils.clp_config import StorageEngine, StorageType

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    dump_container_config,
    generate_container_config,
    generate_container_name,
    generate_container_start_cmd,
    get_clp_home,
    JobType,
    load_config_file,
    validate_and_load_db_credentials_file,
)

logger = logging.getLogger(__file__)


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Searches the compressed logs.")
    args_parser.add_argument(
        "--config",
        "-c",
        default=str(default_config_file_path),
        help="CLP package configuration file.",
    )
    args_parser.add_argument("wildcard_query", help="Wildcard query.")
    args_parser.add_argument(
        "-t", "--tags", help="Comma-separated list of tags of archives to search."
    )
    args_parser.add_argument(
        "--begin-time",
        type=int,
        help="Time range filter lower-bound (inclusive) as milliseconds from the UNIX epoch.",
    )
    args_parser.add_argument(
        "--end-time",
        type=int,
        help="Time range filter upper-bound (inclusive) as milliseconds from the UNIX epoch.",
    )
    args_parser.add_argument(
        "--ignore-case",
        action="store_true",
        help="Ignore case distinctions between values in the query and the compressed data.",
    )
    args_parser.add_argument("--file-path", help="File to search.")
    args_parser.add_argument("--count", action="store_true", help="Count the number of results.")
    args_parser.add_argument(
        "--count-by-time",
        type=int,
        help="Count the number of results in each time span of the given size (ms).",
    )
    args_parser.add_argument(
        "--raw", action="store_true", help="Output the search results as raw logs."
    )
    parsed_args = args_parser.parse_args(argv[1:])

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_logs_dir()

        # Validate and load necessary credentials
        validate_and_load_db_credentials_file(clp_config, clp_home, False)
    except:
        logger.exception("Failed to load config.")
        return -1

    storage_type = clp_config.archive_output.storage.type
    storage_engine = clp_config.package.storage_engine
    if StorageType.S3 == storage_type and StorageEngine.CLP == storage_engine:
        logger.error(
            f"Search is not supported for archive storage type `{storage_type}` with storage engine"
            f" `{storage_engine}`."
        )
        return -1

    container_name = generate_container_name(str(JobType.SEARCH))

    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, container_name
    )

    necessary_mounts = [mounts.clp_home, mounts.logs_dir]
    container_start_cmd = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.execution_container
    )

    # fmt: off
    search_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.search",
        "--config", str(generated_config_path_on_container),
        parsed_args.wildcard_query,
    ]
    # fmt: on
    if parsed_args.tags:
        search_cmd.append("--tags")
        search_cmd.append(parsed_args.tags)
    if parsed_args.begin_time is not None:
        search_cmd.append("--begin-time")
        search_cmd.append(str(parsed_args.begin_time))
    if parsed_args.end_time is not None:
        search_cmd.append("--end-time")
        search_cmd.append(str(parsed_args.end_time))
    if parsed_args.ignore_case:
        search_cmd.append("--ignore-case")
    if parsed_args.file_path:
        search_cmd.append("--file-path")
        search_cmd.append(parsed_args.file_path)
    if parsed_args.count:
        search_cmd.append("--count")
    if parsed_args.count_by_time is not None:
        search_cmd.append("--count-by-time")
        search_cmd.append(str(parsed_args.count_by_time))
    if parsed_args.raw:
        search_cmd.append("--raw")
    cmd = container_start_cmd + search_cmd
    subprocess.run(cmd, check=True)

    # Remove generated files
    generated_config_path_on_host.unlink()

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
