import argparse
import logging
import pathlib
import shlex
import subprocess
import sys

from clp_py_utils.clp_config import (
    CLP_DB_PASS_ENV_VAR_NAME,
    CLP_DB_USER_ENV_VAR_NAME,
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    CLP_DEFAULT_DATASET_NAME,
    ClpDbUserType,
    StorageEngine,
    StorageType,
)
from clp_py_utils.core import resolve_host_path_in_container

from clp_package_utils.general import (
    dump_container_config,
    generate_container_config,
    generate_container_name,
    generate_container_start_cmd,
    get_clp_home,
    get_container_config_filename,
    JobType,
    load_config_file,
    validate_and_load_db_credentials_file,
    validate_dataset_name,
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
    args_parser.add_argument(
        "--verbose",
        "-v",
        action="store_true",
        help="Enable debug logging.",
    )
    args_parser.add_argument("wildcard_query", help="Wildcard query.")
    args_parser.add_argument(
        "--datasets",
        type=str,
        nargs="+",
        default=None,
        help="The datasets that the archives belong to.",
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

    if parsed_args.count and parsed_args.count_by_time is not None:
        logger.error("--count and --count-by-time are mutually exclusive.")
        return -1

    if parsed_args.verbose:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = load_config_file(resolve_host_path_in_container(config_file_path))
        clp_config.validate_logs_dir(True)

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

    datasets = parsed_args.datasets
    if StorageEngine.CLP_S == storage_engine:
        datasets = [CLP_DEFAULT_DATASET_NAME] if datasets is None else datasets
        try:
            clp_db_connection_params = clp_config.database.get_clp_connection_params_and_type(True)
            for ds in datasets:
                validate_dataset_name(clp_db_connection_params["table_prefix"], ds)
        except Exception as e:
            logger.error(e)
            return -1
    elif datasets is not None:
        logger.error(f"Dataset selection is not supported for storage engine: {storage_engine}.")
        return -1

    container_name = generate_container_name(str(JobType.SEARCH))

    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, get_container_config_filename(container_name)
    )
    necessary_mounts = [mounts.logs_dir]
    credentials = clp_config.database.credentials
    extra_env_vars = {
        CLP_DB_PASS_ENV_VAR_NAME: credentials[ClpDbUserType.CLP].password,
        CLP_DB_USER_ENV_VAR_NAME: credentials[ClpDbUserType.CLP].username,
    }
    container_start_cmd = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.container_image_ref, extra_env_vars
    )

    # fmt: off
    search_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.search",
        "--config", str(generated_config_path_on_container),
        parsed_args.wildcard_query,
    ]
    # fmt: on
    if parsed_args.verbose:
        search_cmd.append("--verbose")
    if datasets is not None:
        search_cmd.append("--datasets")
        search_cmd.extend(datasets)
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

    proc = subprocess.run(cmd, check=False)
    ret_code = proc.returncode
    if 0 != ret_code:
        logger.error("Search failed.")
        logger.debug(f"Docker command failed: {shlex.join(cmd)}")

    # Remove generated files
    resolved_generated_config_path_on_host = resolve_host_path_in_container(
        generated_config_path_on_host
    )
    resolved_generated_config_path_on_host.unlink()

    return ret_code


if "__main__" == __name__:
    sys.exit(main(sys.argv))
