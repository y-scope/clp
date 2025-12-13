import argparse
import logging
import shutil
import sys
from contextlib import closing
from pathlib import Path

from clp_py_utils.clp_config import (
    ArchiveOutput,
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    Database,
    S3Config,
    StorageType,
)
from clp_py_utils.clp_metadata_db_utils import (
    delete_dataset_from_metadata_db,
    get_datasets_table_name,
)
from clp_py_utils.s3_utils import s3_delete_by_key_prefix
from clp_py_utils.sql_adapter import SqlAdapter

from clp_package_utils.general import (
    ClpConfig,
    get_clp_home,
    load_config_file,
)
from clp_package_utils.scripts.dataset_manager import (
    DEL_COMMAND,
    LIST_COMMAND,
)

logger: logging.Logger = logging.getLogger(__file__)


def _get_dataset_info(
    db_config: Database,
) -> dict[str, str]:
    """
    :param db_config:
    :return: A map of name -> archive_storage_directory for each dataset that exists.
    """
    sql_adapter = SqlAdapter(db_config)
    with (
        closing(sql_adapter.create_connection(True)) as db_conn,
        closing(db_conn.cursor(dictionary=True)) as db_cursor,
    ):
        clp_db_connection_params = db_config.get_clp_connection_params_and_type(True)
        table_prefix = clp_db_connection_params["table_prefix"]
        db_cursor.execute(
            f"SELECT name, archive_storage_directory FROM `{get_datasets_table_name(table_prefix)}`"
        )
        rows = db_cursor.fetchall()
        return {row["name"]: row["archive_storage_directory"] for row in rows}


def _handle_list_datasets(datasets: dict[str, str]) -> int:
    logger.info(f"Found {len(datasets)} datasets.")
    for dataset in datasets:
        logger.info(dataset)
    return 0


def _handle_del_datasets(
    clp_config: ClpConfig,
    parsed_args: argparse.Namespace,
    existing_datasets_info: dict[str, str],
):
    if len(existing_datasets_info) == 0:
        logger.warning("No datasets exist.")
        return 0

    datasets_to_delete: dict[str, str] = {}
    if parsed_args.del_all:
        datasets_to_delete = existing_datasets_info
    else:
        datasets = parsed_args.datasets
        for dataset in datasets:
            if dataset not in existing_datasets_info:
                logger.error(f"Dataset `{dataset}` doesn't exist. Aborting deletion.")
                return -1

        datasets_to_delete = {dataset: existing_datasets_info[dataset] for dataset in datasets}

    for dataset, dataset_archive_storage_dir in datasets_to_delete.items():
        if not _delete_dataset(clp_config, dataset, dataset_archive_storage_dir):
            return -1

    return 0


def _delete_dataset(clp_config: ClpConfig, dataset: str, dataset_archive_storage_dir: str) -> bool:
    try:
        _try_deleting_archives(clp_config.archive_output, dataset_archive_storage_dir)
        logger.info(f"Deleted archives of dataset `{dataset}`.")
    except:
        logger.exception(f"Failed to delete archives of dataset `{dataset}`.")
        return False

    try:
        _delete_dataset_from_database(clp_config.database, dataset)
        logger.info(f"Deleted dataset `{dataset}` from the metadata database.")
    except:
        logger.exception(f"Failed to delete dataset `{dataset}` from the metadata database.")
        return False

    return True


def _try_deleting_archives(
    archive_output_config: ArchiveOutput, dataset_archive_storage_dir: str
) -> None:
    archive_storage_config = archive_output_config.storage
    storage_type = archive_storage_config.type
    if StorageType.FS == storage_type:
        _try_deleting_archives_from_fs(dataset_archive_storage_dir)
    elif StorageType.S3 == storage_type:
        _try_deleting_archives_from_s3(
            archive_storage_config.s3_config, dataset_archive_storage_dir
        )
    else:
        raise ValueError(f"Unsupported storage type: {storage_type}")


def _try_deleting_archives_from_fs(dataset_archive_storage_dir: str) -> None:
    dataset_archive_storage_path = Path(dataset_archive_storage_dir).resolve()

    if not dataset_archive_storage_path.exists():
        logger.debug(f"'{dataset_archive_storage_path}' doesn't exist.")
        return

    if not dataset_archive_storage_path.is_dir():
        logger.debug(f"'{dataset_archive_storage_path}' isn't a directory. Skipping deletion.")
        return

    shutil.rmtree(dataset_archive_storage_path)


def _try_deleting_archives_from_s3(s3_config: S3Config, archive_storage_key_prefix: str) -> None:
    # Add trailing '/' to avoid deleting other datasets with similar prefixes
    if not archive_storage_key_prefix.endswith("/"):
        archive_storage_key_prefix += "/"

    s3_delete_by_key_prefix(
        s3_config.endpoint_url,
        s3_config.region_code,
        s3_config.bucket,
        archive_storage_key_prefix,
        s3_config.aws_authentication,
    )


def _delete_dataset_from_database(database_config: Database, dataset: str) -> None:
    sql_adapter = SqlAdapter(database_config)

    with (
        closing(sql_adapter.create_connection(True)) as db_conn,
        closing(db_conn.cursor(dictionary=True)) as db_cursor,
    ):
        clp_db_connection_params = database_config.get_clp_connection_params_and_type(True)
        table_prefix = clp_db_connection_params["table_prefix"]
        delete_dataset_from_metadata_db(db_cursor, table_prefix, dataset)
        db_conn.commit()


def main(argv: list[str]) -> int:
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    # Top-level parser and options
    args_parser = argparse.ArgumentParser(description="List or delete datasets.")
    args_parser.add_argument(
        "--config",
        "-c",
        default=str(default_config_file_path),
        help="CLP configuration file.",
    )
    args_parser.add_argument(
        "--verbose",
        "-v",
        action="store_true",
        help="Enable debug logging.",
    )

    # Top-level commands
    subparsers = args_parser.add_subparsers(
        dest="subcommand",
        required=True,
    )
    subparsers.add_parser(
        LIST_COMMAND,
        help="List existing datasets.",
    )

    # Options for delete subcommand
    del_parser = subparsers.add_parser(
        DEL_COMMAND,
        help="Delete datasets from the database and file storage.",
    )
    del_parser.add_argument(
        "datasets",
        nargs="*",
        help="Datasets to delete.",
    )
    del_parser.add_argument(
        "-a",
        "--all",
        dest="del_all",
        action="store_true",
        help="Delete all existing datasets.",
    )

    parsed_args = args_parser.parse_args(argv[1:])
    if parsed_args.verbose:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    # Validate and load config file
    config_file_path = Path(parsed_args.config)
    try:
        clp_config = load_config_file(config_file_path)
        clp_config.validate_logs_dir()
        clp_config.database.load_credentials_from_env()
    except:
        logger.exception("Failed to load config.")
        return -1

    try:
        existing_datasets_info = _get_dataset_info(clp_config.database)
    except:
        logger.exception("Failed to fetch datasets from the database.")
        return -1

    if LIST_COMMAND == parsed_args.subcommand:
        return _handle_list_datasets(existing_datasets_info)
    if DEL_COMMAND == parsed_args.subcommand:
        return _handle_del_datasets(clp_config, parsed_args, existing_datasets_info)
    logger.error(f"Unsupported subcommand: `{parsed_args.subcommand}`.")
    return -1


if "__main__" == __name__:
    sys.exit(main(sys.argv))
