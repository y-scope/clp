import argparse
import logging
import subprocess
import sys
import typing
from pathlib import Path

from clp_py_utils.clp_config import StorageType

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    CLPConfig,
    CLPDockerMounts,
    dump_container_config,
    generate_container_config,
    generate_container_name,
    generate_container_start_cmd,
    get_clp_home,
    load_config_file,
    validate_and_load_db_credentials_file,
)

# Command/Argument Constants
from clp_package_utils.scripts.native.archive_manager import (
    BEGIN_TS_ARG,
    DEL_BY_FILTER_SUBCOMMAND,
    DEL_BY_IDS_SUBCOMMAND,
    DEL_COMMAND,
    DRY_RUN_ARG,
    END_TS_ARG,
    FIND_COMMAND,
)

logger: logging.Logger = logging.getLogger(__file__)


def _validate_timestamps(begin_ts: int, end_ts: typing.Optional[int]) -> bool:
    if begin_ts < 0:
        logger.error("begin-ts must be non-negative.")
        return False
    if end_ts is not None and end_ts < 0:
        logger.error("end-ts must be non-negative.")
        return False
    if end_ts is not None and begin_ts > end_ts:
        logger.error("begin-ts must be <= end-ts.")
        return False
    return True


def main(argv: typing.List[str]) -> int:
    clp_home: Path = get_clp_home()
    default_config_file_path: Path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    # Top-level parser and options
    args_parser: argparse.ArgumentParser = argparse.ArgumentParser(
        description="Views the list of archive IDs or deletes compressed archives."
    )
    args_parser.add_argument(
        "--config",
        "-c",
        default=str(default_config_file_path),
        help="CLP package configuration file.",
    )

    # Top-level commands
    subparsers: argparse._SubParsersAction[argparse.ArgumentParser] = args_parser.add_subparsers(
        dest="subcommand",
        required=True,
    )
    find_parser: argparse.ArgumentParser = subparsers.add_parser(
        FIND_COMMAND,
        help="Lists IDs of compressed archives.",
    )
    del_parser: argparse.ArgumentParser = subparsers.add_parser(
        DEL_COMMAND,
        help="Deletes compressed archives from the database and file system.",
    )

    # Options for find subcommand
    find_parser.add_argument(
        BEGIN_TS_ARG,
        dest="begin_ts",
        type=int,
        default=0,
        help="Time-range lower-bound (inclusive) as milliseconds from the UNIX epoch.",
    )
    find_parser.add_argument(
        END_TS_ARG,
        dest="end_ts",
        type=int,
        help="Time-range upper-bound (inclusive) as milliseconds from the UNIX epoch.",
    )

    # Options for delete subcommand
    del_parser.add_argument(
        DRY_RUN_ARG,
        dest="dry_run",
        action="store_true",
        help="Only prints the archives to be deleted, without actually deleting them.",
    )

    # Subcommands for delete subcommand
    del_subparsers: argparse._SubParsersAction[argparse.ArgumentParser] = del_parser.add_subparsers(
        dest="del_subcommand",
        required=True,
    )

    # Delete by ID subcommand
    del_id_parser: argparse.ArgumentParser = del_subparsers.add_parser(
        DEL_BY_IDS_SUBCOMMAND,
        help="Deletes archives by ID.",
    )

    # Delete by ID arguments
    del_id_parser.add_argument(
        "ids",
        nargs="+",
        help="List of archive IDs to delete",
    )

    # Delete by filter subcommand
    del_filter_parser: argparse.ArgumentParser = del_subparsers.add_parser(
        DEL_BY_FILTER_SUBCOMMAND,
        help="Deletes compressed archives that fall within the specified time range.",
    )

    # Delete by filter arguments
    del_filter_parser.add_argument(
        BEGIN_TS_ARG,
        type=int,
        default=0,
        help="Time-range lower-bound (inclusive) as milliseconds from the UNIX epoch.",
    )
    del_filter_parser.add_argument(
        END_TS_ARG,
        type=int,
        required=True,
        help="Time-range upper-bound (inclusive) as milliseconds from the UNIX epoch.",
    )

    parsed_args: argparse.Namespace = args_parser.parse_args(argv[1:])

    begin_timestamp: typing.Optional[int]
    end_timestamp: typing.Optional[int]
    subcommand: str = parsed_args.subcommand

    # Validate and load config file
    try:
        config_file_path: Path = Path(parsed_args.config)
        clp_config: CLPConfig = load_config_file(
            config_file_path, default_config_file_path, clp_home
        )
        clp_config.validate_logs_dir()

        # Validate and load necessary credentials
        validate_and_load_db_credentials_file(clp_config, clp_home, False)
    except:
        logger.exception("Failed to load config.")
        return -1

    storage_type: StorageType = clp_config.archive_output.storage.type
    if StorageType.FS != storage_type:
        logger.error(f"Archive deletion is not supported for storage type: {storage_type}.")
        return -1

    # Validate input depending on subcommands
    if (DEL_COMMAND == subcommand and DEL_BY_FILTER_SUBCOMMAND == parsed_args.del_subcommand) or (
        FIND_COMMAND == subcommand
    ):
        begin_timestamp = parsed_args.begin_ts
        end_timestamp = parsed_args.end_ts

        # Validate the input timestamp
        assert begin_timestamp is not None, "begin_timestamp is None."
        if not _validate_timestamps(begin_timestamp, end_timestamp):
            return -1

    container_name: str = generate_container_name("archive-manager")

    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, container_name
    )

    necessary_mounts: typing.List[CLPDockerMounts] = [
        mounts.clp_home,
        mounts.logs_dir,
        mounts.archives_output_dir,
    ]
    container_start_cmd: typing.List[str] = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.execution_container
    )

    # fmt: off
    archive_manager_cmd: typing.List[str] = [
        "python3",
        "-m", "clp_package_utils.scripts.native.archive_manager",
        "--config", str(generated_config_path_on_container),
        str(subcommand),
    ]
    # fmt : on

    # Add subcommand-specific arguments
    if DEL_COMMAND == subcommand:
        if parsed_args.dry_run:
            archive_manager_cmd.append(DRY_RUN_ARG)
        if DEL_BY_IDS_SUBCOMMAND == parsed_args.del_subcommand:
            archive_manager_cmd.append(DEL_BY_IDS_SUBCOMMAND)
            archive_manager_cmd.extend(parsed_args.ids)
        elif DEL_BY_FILTER_SUBCOMMAND == parsed_args.del_subcommand:
            archive_manager_cmd.extend([
                DEL_BY_FILTER_SUBCOMMAND,
                str(begin_timestamp),
                str(end_timestamp)
            ])
        else:
            logger.error(f"Unsupported subcommand: `{parsed_args.del_subcommand}`.")
            return -1
    elif FIND_COMMAND == subcommand:
        assert begin_timestamp is not None, "begin_timestamp is None."
        archive_manager_cmd.extend([BEGIN_TS_ARG, str(begin_timestamp)])
        if end_timestamp is not None:
            archive_manager_cmd.extend([END_TS_ARG, str(end_timestamp)])
    else:
        logger.error(f"Unsupported subcommand: `{subcommand}`.")

    cmd: typing.List[str] = container_start_cmd + archive_manager_cmd

    subprocess.run(cmd, check=True)

    # Remove generated files
    generated_config_path_on_host.unlink()

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
