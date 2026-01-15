import argparse
import logging
import shutil
import sys
from abc import ABC, abstractmethod
from contextlib import closing
from pathlib import Path
from typing import Any

from clp_py_utils.clp_config import CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH, Database
from clp_py_utils.clp_metadata_db_utils import (
    delete_archives_from_metadata_db,
    get_archives_table_name,
)
from clp_py_utils.sql_adapter import SqlAdapter

from clp_package_utils.general import (
    ClpConfig,
    get_clp_home,
    load_config_file,
)
from clp_package_utils.scripts.archive_manager import (
    BEGIN_TS_ARG,
    DEL_BY_FILTER_SUBCOMMAND,
    DEL_BY_IDS_SUBCOMMAND,
    DEL_COMMAND,
    DRY_RUN_ARG,
    END_TS_ARG,
    FIND_COMMAND,
)
from clp_package_utils.scripts.native.utils import validate_dataset_exists

logger: logging.Logger = logging.getLogger(__file__)


class DeleteHandler(ABC):
    def __init__(self, query_params: list[str]):
        self._params: list[str] = query_params

    def get_params(self) -> list[str]:
        return self._params

    @abstractmethod
    def get_criteria(self) -> str: ...

    @abstractmethod
    def get_not_found_message(self) -> str: ...

    @abstractmethod
    def validate_results(self, archive_ids: list[str]) -> None: ...


class FilterDeleteHandler(DeleteHandler):
    def get_criteria(self) -> str:
        return "begin_timestamp >= %s AND end_timestamp <= %s"

    def get_not_found_message(self) -> str:
        return "No archives found within the specified time range."

    def validate_results(self, archive_ids: list[str]) -> None:
        pass


class IdDeleteHandler(DeleteHandler):
    def get_criteria(self) -> str:
        placeholders: str = ",".join(["%s"] * len(self._params))
        return f"id in ({placeholders})"

    def get_not_found_message(self) -> str:
        return "No archives found with matching IDs."

    def validate_results(self, archive_ids: list[str]) -> None:
        not_found_ids: set[str] = set(self._params) - set(archive_ids)
        if not_found_ids:
            logger.warning(
                f"""
                Archives with the following IDs don't exist:
                {", ".join(not_found_ids)}
                """
            )


def main(argv: list[str]) -> int:
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
        "--verbose",
        "-v",
        action="store_true",
        help="Enable debug logging.",
    )
    args_parser.add_argument(
        "--dataset",
        type=str,
        default=None,
        help="The dataset that the archives belong to.",
    )

    # Top-level commands
    subparsers: argparse._SubParsersAction[argparse.ArgumentParser] = args_parser.add_subparsers(
        dest="subcommand",
        required=True,
    )
    find_parser: argparse.ArgumentParser = subparsers.add_parser(
        FIND_COMMAND,
        help="List IDs of archives.",
    )
    del_parser: argparse.ArgumentParser = subparsers.add_parser(
        DEL_COMMAND,
        help="Delete archives from the database and file system.",
    )

    # Options for find subcommand
    find_parser.add_argument(
        BEGIN_TS_ARG,
        dest="begin_ts",
        type=int,
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
        help="Preview delete without making changes. Lists errors and files to be deleted.",
    )

    # Subcommands for delete subcommands
    del_subparsers: argparse._SubParsersAction[argparse.ArgumentParser] = del_parser.add_subparsers(
        dest="del_subcommand",
        required=True,
    )

    # Delete by ID subcomand
    del_id_parser: argparse.ArgumentParser = del_subparsers.add_parser(
        DEL_BY_IDS_SUBCOMMAND,
        help="Delete archives by ID.",
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
        help="Deletes archives that fall within the specified time range.",
    )

    # Delete by filter arguments
    del_filter_parser.add_argument(
        "begin_ts",
        type=int,
        help="Time-range lower-bound (inclusive) as milliseconds from the UNIX epoch.",
    )
    del_filter_parser.add_argument(
        "end_ts",
        type=int,
        help="Time-range upper-bound (inclusive) as milliseconds from the UNIX epoch.",
    )

    parsed_args: argparse.Namespace = args_parser.parse_args(argv[1:])
    if parsed_args.verbose:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    # Validate and load config file
    config_file_path: Path = Path(parsed_args.config)
    try:
        clp_config: ClpConfig = load_config_file(config_file_path)
        clp_config.validate_logs_dir()
        clp_config.database.load_credentials_from_env()
    except:
        logger.exception("Failed to load config.")
        return -1

    database_config: Database = clp_config.database
    dataset = parsed_args.dataset
    if dataset is not None:
        try:
            validate_dataset_exists(database_config, dataset)
        except Exception as e:
            logger.error(e)
            return -1

    archives_dir: Path = clp_config.archive_output.get_directory()
    if not archives_dir.exists():
        logger.error("`archive_output.directory` doesn't exist.")
        return -1

    if FIND_COMMAND == parsed_args.subcommand:
        return _find_archives(
            archives_dir,
            database_config,
            dataset,
            parsed_args.begin_ts,
            parsed_args.end_ts,
        )
    if DEL_COMMAND == parsed_args.subcommand:
        delete_handler: DeleteHandler
        if DEL_BY_IDS_SUBCOMMAND == parsed_args.del_subcommand:
            delete_handler: IdDeleteHandler = IdDeleteHandler(parsed_args.ids)
            return _delete_archives(
                archives_dir,
                database_config,
                dataset,
                delete_handler,
                parsed_args.dry_run,
            )
        if DEL_BY_FILTER_SUBCOMMAND == parsed_args.del_subcommand:
            delete_handler: FilterDeleteHandler = FilterDeleteHandler(
                [parsed_args.begin_ts, parsed_args.end_ts]
            )
            return _delete_archives(
                archives_dir,
                database_config,
                dataset,
                delete_handler,
                parsed_args.dry_run,
            )
        logger.error(f"Unsupported subcommand: `{parsed_args.del_subcommand}`.")
        return -1
    logger.error(f"Unsupported subcommand: `{parsed_args.subcommand}`.")
    return -1


def _find_archives(
    archives_dir: Path,
    database_config: Database,
    dataset: str | None,
    begin_ts: int,
    end_ts: int,
) -> int:
    """
    Lists all archive IDs, if begin_ts and end_ts are provided, only lists archives where
    `begin_ts <= archive.begin_timestamp` and `archive.end_timestamp <= end_ts`.
    :param archives_dir:
    :param database_config:
    :param dataset:
    :param begin_ts:
    :param end_ts:
    :return: 0 on success, 1 on failure.
    """
    archive_ids: list[str]
    dataset_specific_message = f" of dataset `{dataset}`" if dataset is not None else ""
    logger.info(f"Starting to find archives{dataset_specific_message} from the database.")
    try:
        sql_adapter: SqlAdapter = SqlAdapter(database_config)
        clp_db_connection_params: dict[str, Any] = (
            database_config.get_clp_connection_params_and_type(True)
        )
        table_prefix: str = clp_db_connection_params["table_prefix"]

        with (
            closing(sql_adapter.create_connection(True)) as db_conn,
            closing(db_conn.cursor(dictionary=True)) as db_cursor,
        ):
            query_params: list[int] = [begin_ts]
            query: str = f"""
                SELECT id FROM `{get_archives_table_name(table_prefix, dataset)}`
                WHERE begin_timestamp >= %s
                """
            if end_ts is not None:
                query += " AND end_timestamp <= %s"
                query_params.append(end_ts)

            db_cursor.execute(query, query_params)
            results = db_cursor.fetchall()

            archive_ids: list[str] = [result["id"] for result in results]
            if 0 == len(archive_ids):
                logger.info("No archives found within specified time range.")
                return 0

            logger.info(f"Found {len(archive_ids)} archives within the specified time range.")
            archive_output_dir = archives_dir / dataset if dataset is not None else archives_dir
            for archive_id in archive_ids:
                logger.info(archive_id)
                archive_path = archive_output_dir / archive_id
                if not archive_path.is_dir():
                    logger.warning(f"Archive {archive_id} in database not found on disk.")

    except Exception:
        logger.exception("Failed to find archives from the database.")
        return -1

    logger.info("Finished finding archives from the database.")
    return 0


def _delete_archives(
    archives_dir: Path,
    database_config: Database,
    dataset: str | None,
    delete_handler: DeleteHandler,
    dry_run: bool = False,
) -> int:
    """
    Deletes archives from both metadata database and disk based on provided SQL query.

    :param archives_dir:
    :param database_config:
    :param dataset:
    :param delete_handler: Object to handle differences between by-filter and by-ids delete types.
    :param dry_run: If True, no changes will be made to the database or disk.
    :return: 0 on success, -1 otherwise.
    """
    archive_ids: list[str]
    dataset_specific_message = f" of dataset `{dataset}`" if dataset is not None else ""
    logger.info(f"Starting to delete archives{dataset_specific_message} from the database.")
    sql_adapter: SqlAdapter = SqlAdapter(database_config)
    clp_db_connection_params: dict[str, Any] = database_config.get_clp_connection_params_and_type(
        True
    )
    table_prefix = clp_db_connection_params["table_prefix"]

    try:
        with (
            closing(sql_adapter.create_connection(True)) as db_conn,
            closing(db_conn.cursor(dictionary=True)) as db_cursor,
        ):
            if dry_run:
                logger.info("Running in dry-run mode.")

            query_criteria: str = delete_handler.get_criteria()
            query_params: list[str] = delete_handler.get_params()

            db_cursor.execute(
                f"""
                SELECT id FROM `{get_archives_table_name(table_prefix, dataset)}`
                WHERE {query_criteria}
                """,
                query_params,
            )
            results = db_cursor.fetchall()

            if 0 == len(results):
                logger.info(delete_handler.get_not_found_message())
                return 0

            archive_ids: list[str] = [result["id"] for result in results]
            delete_handler.validate_results(archive_ids)

            delete_archives_from_metadata_db(db_cursor, archive_ids, table_prefix, dataset)
            for archive_id in archive_ids:
                logger.info(f"Deleted archive {archive_id} from the database.")

            if dry_run:
                logger.info("Dry-run finished.")
                db_conn.rollback()
                return 0

            db_conn.commit()

    except Exception:
        logger.exception("Failed to delete archives from the database. Aborting deletion.")
        return -1

    logger.info("Finished deleting archives from the database.")

    archive_output_dir: Path = archives_dir / dataset if dataset is not None else archives_dir
    for archive_id in archive_ids:
        archive_path = archive_output_dir / archive_id
        if not archive_path.is_dir():
            logger.warning(f"Archive {archive_id} is not a directory. Skipping deletion.")
            continue

        logger.info(f"Deleting archive {archive_id} from disk.")
        shutil.rmtree(archive_path)

    logger.info("Finished deleting archives from disk.")

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
