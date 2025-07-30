import argparse
import logging
import shutil
import sys
from contextlib import closing
from pathlib import Path
from typing import Dict, List

from clp_py_utils.clp_config import ArchiveOutput, Database, StorageType
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
LIST_COMMAND: str = "list"
DEL_COMMAND: str = "del"

logger: logging.Logger = logging.getLogger(__file__)


def _handle_list_datasets(datasets: Dict[str, str]) -> int:
    logger.info(f"Found {len(datasets)} datasets.")
    for dataset_name, _ in datasets.items():
        logger.info(dataset_name)
    return 0


def _fetch_existing_datasets_info(
    db_config: Database,
) -> Dict[str, str]:
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
    datasets_to_delete: List[str] = []
    if parsed_args.del_all:
        datasets_to_delete = list(existing_datasets_info.keys())
    else:
        datasets = parsed_args.datasets
        for dataset in datasets:
            if dataset not in existing_datasets_info:
                logger.error(f"Dataset `{dataset}` doesn't exist.")
                continue
            datasets_to_delete.append(dataset)

    if 0 == len(datasets_to_delete):
        logger.warning("No dataset will be deleted...")
        return 0

    for dataset in datasets_to_delete:
        if not _delete_dataset(dataset, existing_datasets_info[dataset], clp_config):
            return -1

    return 0


def _delete_dataset(dataset: str, dataset_archive_storage_dir: str, clp_config: CLPConfig) -> bool:
    try:
        _try_deleting_archives(dataset_archive_storage_dir, clp_config.archive_output)
        logger.info(f"Successfully removed archives for dataset `{dataset}`.")
    except:
        logger.exception(f"Failed to remove archives for dataset `{dataset}`, abort...")
        return False

    try:
        _delete_dataset_from_database(dataset, clp_config.database)
        logger.info(f"Successfully removed dataset `{dataset}` from database.")
    except:
        logger.exception(f"Failed to remove dataset `{dataset}` from database, abort...")
        return False

    return True


def _delete_dataset_from_database(dataset: str, database_config: Database):
    sql_adapter: SQL_Adapter = SQL_Adapter(database_config)
    clp_db_connection_params: dict[str, any] = database_config.get_clp_connection_params_and_type(
        True
    )

    table_prefix = clp_db_connection_params["table_prefix"]

    # Drop tables in the order such that no foreign key constraint is violated.
    tables_remove_order = [
        get_column_metadata_table_name(table_prefix, dataset),
        get_files_table_name(table_prefix, dataset),
        get_archive_tags_table_name(table_prefix, dataset),
        get_tags_table_name(table_prefix, dataset),
        get_archives_table_name(table_prefix, dataset),
    ]

    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        for table in tables_remove_order:
            db_cursor.execute(
                f"""
                DROP TABLE `{table}`
                """
            )

        db_cursor.execute(
            f"""
            DELETE FROM `{get_datasets_table_name(table_prefix)}`
            WHERE name = %s
            """,
            (dataset,),
        )
        db_conn.commit()


def _try_deleting_archives(
    dataset_archive_storage_dir: str, archive_output_config: ArchiveOutput
) -> None:
    archive_storage_config = archive_output_config.storage
    storage_type = archive_storage_config.type
    if StorageType.S3 == storage_type:
        s3_config = archive_storage_config.s3_config
        # Add trailing '/' to avoid deleting other datasets with similar prefixes
        if not dataset_archive_storage_dir.endswith("/"):
            dataset_archive_storage_dir += "/"

        s3_delete_by_key_prefix(
            s3_config.region_code,
            s3_config.bucket,
            dataset_archive_storage_dir,
            s3_config.aws_authentication,
        )

    elif StorageType.FS == storage_type:
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

    else:
        raise ValueError(f"Unsupported storage type: {storage_type}")


def main(argv: List[str]) -> int:
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    # Top-level parser and options
    args_parser = argparse.ArgumentParser(
        description="View list of archive IDs or delete compressed archives."
    )
    args_parser.add_argument(
        "--config",
        "-c",
        default=str(default_config_file_path),
        help="CLP configuration file.",
    )
    args_parser.add_argument(
        "--dataset",
        type=str,
        default=None,
        help="The dataset that the archives belong to.",
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
        help="Deletes datasets from the database and file system.",
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
    except Exception:
        logger.exception("Failed to fetch datasets from the database.")
        return -1

    ret_val: int
    if LIST_COMMAND == parsed_args.subcommand:
        ret_val = _handle_list_datasets(existing_datasets_info)
    elif DEL_COMMAND == parsed_args.subcommand:
        ret_val = _handle_del_datasets(clp_config, parsed_args, existing_datasets_info)
    else:
        logger.error(f"Unsupported subcommand: `{parsed_args.subcommand}`.")
        ret_val = -1

    return ret_val


if "__main__" == __name__:
    sys.exit(main(sys.argv))
