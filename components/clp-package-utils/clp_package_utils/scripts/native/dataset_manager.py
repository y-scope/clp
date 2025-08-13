import argparse
import logging
import shutil
import sys
from contextlib import closing
from pathlib import Path
from typing import Dict, List

from clp_py_utils.clp_config import ArchiveOutput, Database, S3Config, StorageType
from clp_py_utils.clp_metadata_db_utils import (
    get_archive_tags_table_name,
    get_archives_table_name,
    get_column_metadata_table_name,
    get_datasets_table_name,
    get_files_table_name,
    get_tags_table_name,
)
from clp_py_utils.s3_utils import s3_delete_by_key_prefix
from clp_py_utils.sql_adapter import SQL_Adapter

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    CLPConfig,
    get_clp_home,
    load_config_file,
)

# Command/Argument Constants
from clp_package_utils.scripts.dataset_manager import (
    DEL_COMMAND,
    LIST_COMMAND,
)

logger: logging.Logger = logging.getLogger(__file__)


def _handle_list_datasets(datasets: Dict[str, str]) -> int:
    logger.info(f"Found {len(datasets)} datasets.")
    for dataset_name, _ in datasets.items():
        logger.info(dataset_name)
    return 0


def _fetch_existing_datasets_info(
    db_config: Database,
) -> Dict[str, str]:
    """
    Retrieves a mapping of existing dataset names to their corresponding archive storage directories.

    :param db_config:
    :return: Dict[str, str] containing the mapping of dataset names to their corresponding archive_storage_directory
    """

    sql_adapter = SQL_Adapter(db_config)
    clp_db_connection_params = db_config.get_clp_connection_params_and_type(True)
    table_prefix = clp_db_connection_params["table_prefix"]
    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        db_cursor.execute(
            f"SELECT name, archive_storage_directory FROM `{get_datasets_table_name(table_prefix)}`"
        )
        rows = db_cursor.fetchall()
        return {row["name"]: row["archive_storage_directory"] for row in rows}


def _handle_del_datasets(
    clp_config: CLPConfig,
    parsed_args: argparse.Namespace,
    existing_datasets_info: Dict[str, str],
):
    if len(existing_datasets_info) == 0:
        logger.warning("No datasets exist in the database. Skip deletion.")
        return 0

    datasets_to_delete: Dict[str, str] = dict()
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
        if not _delete_dataset(dataset, dataset_archive_storage_dir, clp_config):
            return -1

    return 0


def _delete_dataset(dataset: str, dataset_archive_storage_dir: str, clp_config: CLPConfig) -> bool:
    try:
        _try_deleting_archives(clp_config.archive_output, dataset_archive_storage_dir)
        logger.info(f"Successfully deleted archives of dataset `{dataset}`.")
    except:
        logger.exception(f"Failed to delete archives for dataset `{dataset}`, abort...")
        return False

    try:
        _delete_dataset_from_database(clp_config.database, dataset)
        logger.info(f"Successfully deleted dataset `{dataset}` from database.")
    except:
        logger.exception(f"Failed to delete dataset `{dataset}` from database, abort...")
        return False

    return True


def _delete_dataset_from_database(database_config: Database, dataset: str) -> None:
    """
    Deletes all tables associated with the `dataset` from the metadata database.

    :param database_config:
    :param dataset:
    """
    clp_db_connection_params = database_config.get_clp_connection_params_and_type(True)
    table_prefix = clp_db_connection_params["table_prefix"]
    # Drop tables in an order such that no foreign key constraint is violated.
    tables_removal_order = [
        get_column_metadata_table_name(table_prefix, dataset),
        get_files_table_name(table_prefix, dataset),
        get_archive_tags_table_name(table_prefix, dataset),
        get_tags_table_name(table_prefix, dataset),
        get_archives_table_name(table_prefix, dataset),
    ]

    sql_adapter = SQL_Adapter(database_config)
    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        for table in tables_removal_order:
            db_cursor.execute(f"DROP TABLE IF EXISTS `{table}`")

        # Remove the dataset row from the datasets table
        db_cursor.execute(
            f"""
            DELETE FROM `{get_datasets_table_name(table_prefix)}`
            WHERE name = %s
            """,
            (dataset,),
        )
        db_conn.commit()


def _try_deleting_archives_from_s3(s3_config: S3Config, archive_storage_key_prefix: str) -> None:
    # Add trailing '/' to avoid deleting other datasets with similar prefixes
    if not archive_storage_key_prefix.endswith("/"):
        archive_storage_key_prefix += "/"

    s3_delete_by_key_prefix(
        s3_config.region_code,
        s3_config.bucket,
        archive_storage_key_prefix,
        s3_config.aws_authentication,
    )


def _try_deleting_archives_from_fs(
    archive_output_config: ArchiveOutput, dataset_archive_storage_dir: str
) -> None:
    archives_dir = archive_output_config.get_directory()
    dataset_archive_storage_path = Path(dataset_archive_storage_dir).resolve()
    if not dataset_archive_storage_path.is_relative_to(archives_dir):
        raise ValueError(
            f"Fatal: {dataset_archive_storage_path} is not within top-level archive storage directory {archives_dir}"
        )

    if not dataset_archive_storage_path.exists():
        logger.debug(f"{dataset_archive_storage_path} doesn't exist, skip deletion...")
        return

    if not dataset_archive_storage_path.is_dir():
        logger.debug(
            f"{dataset_archive_storage_path} doesn't resolve to a directory, skip deletion..."
        )
        return

    shutil.rmtree(dataset_archive_storage_path)


def _try_deleting_archives(
    archive_output_config: ArchiveOutput, dataset_archive_storage_dir: str
) -> None:
    archive_storage_config = archive_output_config.storage
    storage_type = archive_storage_config.type
    if StorageType.S3 == storage_type:
        _try_deleting_archives_from_s3(
            archive_storage_config.s3_config, dataset_archive_storage_dir
        )

    elif StorageType.FS == storage_type:
        _try_deleting_archives_from_fs(archive_output_config, dataset_archive_storage_dir)

    else:
        raise ValueError(f"Unsupported storage type: {storage_type}")


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
    if parsed_args.verbose:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    # Validate and load config file
    config_file_path = Path(parsed_args.config)
    try:
        clp_config = load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_logs_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    try:
        existing_datasets_info = _fetch_existing_datasets_info(clp_config.database)
    except:
        logger.exception("Failed to fetch datasets from the database.")
        return -1

    if LIST_COMMAND == parsed_args.subcommand:
        return _handle_list_datasets(existing_datasets_info)
    elif DEL_COMMAND == parsed_args.subcommand:
        return _handle_del_datasets(clp_config, parsed_args, existing_datasets_info)
    else:
        logger.error(f"Unsupported subcommand: `{parsed_args.subcommand}`.")
        return -1


if "__main__" == __name__:
    sys.exit(main(sys.argv))
