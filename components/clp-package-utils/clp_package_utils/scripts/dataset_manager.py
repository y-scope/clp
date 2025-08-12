import argparse
import logging
import subprocess
import sys
from pathlib import Path
from typing import List

from clp_py_utils.clp_config import (
    StorageEngine,
    StorageType,
)

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    dump_container_config,
    generate_container_config,
    generate_container_name,
    generate_container_start_cmd,
    get_clp_home,
    load_config_file,
    validate_and_load_db_credentials_file,
    validate_dataset_name,
)

# Command/Argument Constants
LIST_COMMAND: str = "list"
DEL_COMMAND: str = "del"

logger = logging.getLogger(__file__)


def main(argv: List[str]) -> int:
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    # Top-level parser and options
    args_parser = argparse.ArgumentParser(
        description="Views the list of existing datasets or deletes dataset(s)."
    )
    args_parser.add_argument(
        "--config",
        "-c",
        default=str(default_config_file_path),
        help="CLP package configuration file.",
    )

    # Top-level commands
    subparsers = args_parser.add_subparsers(
        dest="subcommand",
        required=True,
    )
    subparsers.add_parser(
        LIST_COMMAND,
        help="Lists existing datasets.",
    )

    # Options for delete subcommand
    del_parser = subparsers.add_parser(
        DEL_COMMAND,
        help="Deletes dataset(s) from the database and the file system.",
    )
    del_parser.add_argument(
        "datasets",
        nargs="*",
        help="dataset(s) to delete.",
    )
    del_parser.add_argument(
        "-a",
        "--all",
        dest="del_all",
        action="store_true",
        help="Delete all existing datasets.",
    )

    parsed_args = args_parser.parse_args(argv[1:])
    subcommand = parsed_args.subcommand

    # Validate and load config file
    try:
        config_file_path = Path(parsed_args.config)
        clp_config = load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_logs_dir()

        # Validate and load necessary credentials
        validate_and_load_db_credentials_file(clp_config, clp_home, False)
    except:
        logger.exception("Failed to load config.")
        return -1

    storage_engine = clp_config.package.storage_engine
    if StorageEngine.CLP_S != storage_engine:
        logger.error(f"Datasets operation is not supported for storage engine: {storage_engine}.")
        return -1

    # Validate input depending on subcommands
    if DEL_COMMAND == subcommand:
        datasets = parsed_args.datasets
        del_all_flag = parsed_args.del_all
        if not del_all_flag and len(datasets) == 0:
            args_parser.error("No dataset name is specified for deletion.")
        if del_all_flag and len(datasets) != 0:
            args_parser.error(
                "The -a/--all flag cannot be used together with specified dataset names."
            )

        try:
            clp_db_connection_params = clp_config.database.get_clp_connection_params_and_type(True)
            table_prefix = clp_db_connection_params["table_prefix"]
            for dataset in datasets:
                validate_dataset_name(table_prefix, dataset)
        except Exception as e:
            logger.error(e)
            return -1

    container_name = generate_container_name("dataset-manager")

    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, container_name
    )

    necessary_mounts = [
        mounts.clp_home,
        mounts.logs_dir,
    ]
    if clp_config.archive_output.storage.type == StorageType.FS:
        necessary_mounts.append(mounts.archives_output_dir)

    container_start_cmd = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.execution_container
    )

    # fmt: off
    dataset_manager_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.dataset_manager",
        "--config", str(generated_config_path_on_container),
    ]
    # fmt : on
    dataset_manager_cmd.append(subcommand)

    # Add subcommand-specific arguments
    if DEL_COMMAND == subcommand:
        if parsed_args.del_all:
            dataset_manager_cmd.append("--all")
        else:
            dataset_manager_cmd.extend(parsed_args.datasets)
    elif LIST_COMMAND != subcommand:
        logger.error(f"Unsupported subcommand: `{subcommand}`.")

    cmd = container_start_cmd + dataset_manager_cmd

    subprocess.run(cmd, check=True)

    # Remove generated files
    generated_config_path_on_host.unlink()

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
