import argparse
import logging
import shutil
import sys
import typing
from contextlib import closing
from pathlib import Path

from clp_py_utils.clp_config import Database, StorageType
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


def _print_datasets_list(datasets: typing.Dict[str, str]) -> None:
    logger.info(f"Found {len(datasets)} datasets.")
    for dataset_name, _ in datasets.items():
        logger.info(dataset_name)


def _fetch_existing_datasets_info(
    db_config: Database,
) -> typing.Dict[str, str]:
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


def _delete_dataset_from_database(dataset: str, clp_config: CLPConfig):
    database_config = clp_config.database
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


def _del_dataset(dataset: str, dataset_archive_storage_dir: str, clp_config: CLPConfig) -> int:
    try:
        _try_deleting_archives(dataset_archive_storage_dir, clp_config)
        logger.info(f"Successfully removed archives for dataset `{dataset}`.")
    except:
        logger.exception(f"Failed to remove archives for dataset `{dataset}`, abort...")
        return -1

    try:
        _delete_dataset_from_database(dataset, clp_config)
        logger.info(f"Successfully removed dataset `{dataset}` from database.")
    except:
        logger.exception(f"Failed to remove dataset `{dataset}` from database, abort...")
        return -1

    return 0


def _try_deleting_archives(dataset_archive_storage_dir: str, clp_config: CLPConfig) -> None:
    archive_storage_config = clp_config.archive_output.storage
    storage_type = archive_storage_config.type
    if StorageType.S3 == storage_type:
        s3_config = archive_storage_config.s3_config
        s3_delete_by_key_prefix(
            s3_config.region_code,
            s3_config.bucket,
            dataset_archive_storage_dir,
            s3_config.aws_authentication,
        )

    elif StorageType.FS == storage_type:
        archives_dir = clp_config.archive_output.get_directory()
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


def main(argv: typing.List[str]) -> int:
    clp_home: Path = get_clp_home()
    default_config_file_path: Path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    # Top-level parser and options
    args_parser: argparse.ArgumentParser = argparse.ArgumentParser(
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
        nargs="+",
        help="dataset(s) to delete.",
    )

    parsed_args: argparse.Namespace = args_parser.parse_args(argv[1:])

    # Validate and load config file
    config_file_path: Path = Path(parsed_args.config)
    try:
        clp_config: CLPConfig = load_config_file(
            config_file_path, default_config_file_path, clp_home
        )
        clp_config.validate_logs_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    database_config: Database = clp_config.database
    try:
        existing_datasets_info = _fetch_existing_datasets_info(database_config)
    except Exception:
        logger.exception("Failed to fetch datasets from the database.")
        return -1

    if LIST_COMMAND == parsed_args.subcommand:
        _print_datasets_list(existing_datasets_info)
        return 0

    elif DEL_COMMAND == parsed_args.subcommand:
        datasets = parsed_args.datasets
        for dataset in datasets:
            if dataset not in existing_datasets_info:
                logger.error(f"Dataset {dataset} doesn't exist")
                continue
            return _del_dataset(dataset, existing_datasets_info[dataset], clp_config)
    else:
        logger.error(f"Unsupported subcommand: `{parsed_args.subcommand}`.")
        return -1


if "__main__" == __name__:
    sys.exit(main(sys.argv))
