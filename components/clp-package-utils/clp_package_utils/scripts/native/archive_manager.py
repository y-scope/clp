import argparse
import logging
import shutil
import sys
from contextlib import closing
from pathlib import Path
from typing import List

from clp_py_utils.clp_config import Database
from clp_py_utils.sql_adapter import SQL_Adapter

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    get_clp_home,
    load_config_file,
)

# Command/Argument Constants
FIND_COMMAND = "find"
DEL_COMMAND = "del"
BY_IDS_COMMAND = "by-ids"
BY_FILTER_COMMAND = "by-filter"
BEGIN_TS_ARG = "--begin-ts"
END_TS_ARG = "--end-ts"
DRY_RUN_ARG = "--dry-run"

logger = logging.getLogger(__file__)


def main(argv):
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

    # Top-level commands
    subparsers = args_parser.add_subparsers(
        dest="subcommand",
        required=True,
    )
    find_parser = subparsers.add_parser(
        FIND_COMMAND,
        help="List IDs of archives.",
    )
    del_parser = subparsers.add_parser(
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
        help="Time-range upper-bound (include) as milliseconds from the UNIX epoch.",
    )

    # Options for delete subcommand
    del_parser.add_argument(
        DRY_RUN_ARG,
        dest="dry_run",
        action="store_true",
        help="Preview delete without making changes. Lists errors and files to be deleted.",
    )

    # Subcommands for delete subcommands
    del_subparsers = del_parser.add_subparsers(
        dest="del_subcommand",
        required=True,
    )

    # Delete by ID subcomand
    del_id_parser = del_subparsers.add_parser(
        BY_IDS_COMMAND,
        help="Delete archives by ID.",
    )

    # Delete by ID arguments
    del_id_parser.add_argument(
        "ids",
        nargs="+",
        help="List of archive IDs to delete",
    )

    # Delete by filter subcommand
    del_filter_parser = del_subparsers.add_parser(
        BY_FILTER_COMMAND,
        help="delete archives within time frame.",
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
        help="Time-range upper-bound (include) as milliseconds from the UNIX epoch.",
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

    database_config = clp_config.database
    archives_dir = clp_config.archive_output.get_directory()
    if not archives_dir.exists():
        logger.error("`archive_output.directory` doesn't exist.")
        return -1

    if FIND_COMMAND == parsed_args.subcommand:
        return _find_archives(
            archives_dir,
            database_config,
            parsed_args.begin_ts,
            parsed_args.end_ts,
        )
    elif DEL_COMMAND == parsed_args.subcommand:
        if BY_IDS_COMMAND == parsed_args.del_subcommand:
            return _delete_archives(
                archives_dir,
                database_config,
                parsed_args.ids,
                BY_IDS_COMMAND,
                parsed_args.dry_run,
            )
        elif BY_FILTER_COMMAND == parsed_args.del_subcommand:
            return _delete_archives(
                archives_dir,
                database_config,
                [parsed_args.begin_ts, parsed_args.end_ts],
                BY_FILTER_COMMAND,
                parsed_args.dry_run,
            )


def _find_archives(
    archives_dir: Path,
    database_config: Database,
    begin_ts: int,
    end_ts: int = None,
) -> int:
    """
    Lists all archive IDs, if begin_its and end_its are provided,
    only lists archives where `begin_ts <= archive.begin_timestamp` and
    `archive.end_timestamp <= end_ts`.
    :param archives_dir:
    :param database_config:
    :param begin_ts:
    :param end_ts:
    """
    archive_ids: List[str]
    logger.info("Starting to find archives from the database.")
    try:
        sql_adapter = SQL_Adapter(database_config)
        clp_db_connection_params = database_config.get_clp_connection_params_and_type(True)
        table_prefix = clp_db_connection_params["table_prefix"]
        with closing(sql_adapter.create_connection(True)) as db_conn, closing(
            db_conn.cursor(dictionary=True)
        ) as db_cursor:
            params = (begin_ts,)
            query = f"SELECT id FROM `{table_prefix}archives` WHERE begin_timestamp >= %s"
            if end_ts is not None:
                query += " AND end_timestamp <= %s"
                params = params + (end_ts,)

            db_cursor.execute(query, params)
            results = db_cursor.fetchall()

            archive_ids = [result["id"] for result in results]
            if 0 == len(archive_ids):
                logger.info("No archives found within specified time range.")
                return 0

            logger.info(f"Found {len(archive_ids)} archives within the specified time range.")
            for archive_id in archive_ids:
                logger.info(archive_id)

    except Exception:
        logger.exception("Failed to find archives from the database.")
        return -1

    logger.info(f"Finished finding archives from the database.")

    for archive_id in archive_ids:
        archive_path = archives_dir / archive_id
        if not archive_path.is_dir():
            logger.warning(f"Archive {archive_id} in database not found on disk.")

    return 0


def _delete_archives(
    archives_dir: Path,
    database_config: Database,
    params: List[str],
    command: str,
    dry_run: bool = False,
) -> int:
    """
    Deletes archives from both metadata database and disk based on provided SQL query.
    :param archives_dir:
    :param database_config:
    :param query: SQL query to select archives to delete.
    :param params: List of parameters for SQL query.
    :param command: Delete subcommand. Either "filter" or "ids".
    :param dry_run: If True, no changes will be made to the database or disk.
    :return: 0 on success, -1 otherwise.
    """

    archive_ids: List[str]
    logger.info("Starting to delete archives from the database.")
    try:
        sql_adapter = SQL_Adapter(database_config)
        clp_db_connection_params = database_config.get_clp_connection_params_and_type(True)
        table_prefix = clp_db_connection_params["table_prefix"]
        with closing(sql_adapter.create_connection(True)) as db_conn, closing(
            db_conn.cursor(dictionary=True)
        ) as db_cursor:
            if dry_run:
                logger.info("Running in dry-run mode.")

            criteria: str
            if BY_FILTER_COMMAND == command:
                criteria = "begin_timestamp >= %s AND end_timestamp <= %s"
            elif BY_IDS_COMMAND == command:
                criteria = "id in (%s)" % ",".join(["%s"] * len(params))

            db_cursor.execute(
                f"""
                DELETE FROM `{table_prefix}archives`
                WHERE {criteria}
                RETURNING id
                """,
                params,
            )
            results = db_cursor.fetchall()

            if 0 == len(results):
                if BY_FILTER_COMMAND == command:
                    logger.info("No archives found within specified time frame.")
                elif BY_IDS_COMMAND == command:
                    logger.info("No archives found with matching IDs.")
                return 0

            archive_ids = [result["id"] for result in results]
            if BY_IDS_COMMAND == command:
                not_found_ids = set(params) - set(archive_ids)
                if not_found_ids:
                    logger.warning(
                        f"""
                        Archives with the following IDs don't exist:
                        {', '.join(not_found_ids)}
                        """
                    )

            ids_string = ", ".join(f"'{archive_id}'" for archive_id in archive_ids)

            db_cursor.execute(
                f"""
                DELETE FROM `{table_prefix}files`
                WHERE archive_id in ({ids_string})
                """
            )

            db_cursor.execute(
                f"""
                DELETE FROM `{table_prefix}archive_tags`
                WHERE archive_id in ({ids_string})
                """
            )
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

    logger.info(f"Finished deleting archives from the database.")

    for archive_id in archive_ids:
        archive_path = archives_dir / archive_id
        if not archive_path.is_dir():
            logger.warning(f"Archive {archive_id} is not a directory. Skipping deletion.")
            continue

        logger.info(f"Deleting archive {archive_id} from disk.")
        shutil.rmtree(archive_path)

    logger.info(f"Finished deleting archives from disk.")

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
