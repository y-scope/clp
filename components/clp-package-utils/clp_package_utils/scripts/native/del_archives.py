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

logger = logging.getLogger(__file__)


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(
        description="Deletes archives that fall within the specified time range."
    )
    args_parser.add_argument(
        "--config",
        "-c",
        required=True,
        default=str(default_config_file_path),
        help="CLP configuration file.",
    )
    args_parser.add_argument(
        "begin_ts",
        type=int,
        help="Time-range lower-bound (inclusive) as milliseconds from the UNIX epoch.",
    )
    args_parser.add_argument(
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

    return _delete_archives(
        archives_dir,
        database_config,
        parsed_args.begin_ts,
        parsed_args.end_ts,
    )


def _delete_archives(
    archives_dir: Path,
    database_config: Database,
    begin_ts: int,
    end_ts: int,
) -> int:
    """
    Deletes all archives where `begin_ts <= archive.begin_timestamp` and
    `archive.end_timestamp <= end_ts` from both the metadata database and disk.
    :param archives_dir:
    :param database_config:
    :param begin_ts:
    :param end_ts:
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
            db_cursor.execute(
                f"""
                DELETE FROM `{table_prefix}archives`
                WHERE begin_timestamp >= %s AND end_timestamp <= %s
                RETURNING id
                """,
                (begin_ts, end_ts),
            )
            results = db_cursor.fetchall()

            if 0 == len(results):
                logger.info("No archives (exclusively) within the specified time range.")
                return 0

            archive_ids = [result["id"] for result in results]
            db_cursor.execute(
                f"""
                DELETE FROM `{table_prefix}files`
                WHERE archive_id in ({', '.join(['%s'] * len(archive_ids))})
                """,
                archive_ids,
            )
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
