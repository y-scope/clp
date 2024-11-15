import argparse
import logging
import pathlib
import shutil
import sys
from contextlib import closing
from typing import List, Optional

from clp_py_utils.clp_config import CLPConfig
from clp_py_utils.sql_adapter import SQL_Adapter

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    get_clp_home,
    load_config_file,
)

# Setup logging
# Create logger
logger = logging.getLogger(__file__)
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] [%(name)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


# Consider deduplicate this
def validate_and_load_config_file(
    clp_home: pathlib.Path,
    config_file_path: pathlib.Path,
    default_config_file_path: pathlib.Path,
) -> Optional[CLPConfig]:
    """
    Validates and loads the config file.
    :param clp_home:
    :param config_file_path:
    :param default_config_file_path:
    :return: clp_config on success, None otherwise.
    """
    try:
        clp_config = load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_archive_output_dir()
        clp_config.validate_logs_dir()
        return clp_config
    except Exception:
        logger.exception("Failed to load config.")
        return None


def handle_file_deletion(
    clp_home: pathlib.Path,
    config_file_path: pathlib.Path,
    default_config_file_path: pathlib.Path,
    begin_ts: int,
    end_ts: int,
) -> int:
    # Validate and load config file
    clp_config = validate_and_load_config_file(clp_home, config_file_path, default_config_file_path)
    if clp_config is None:
        return -1

    archives_dir = clp_config.archive_output.directory
    database_config = clp_config.database
    archives_to_delete: List[str]

    logger.info(f"Start deleting archives from database")
    try:
        sql_adapter = SQL_Adapter(database_config)
        clp_db_connection_params = database_config.get_clp_connection_params_and_type(True)
        table_prefix = clp_db_connection_params["table_prefix"]
        with closing(sql_adapter.create_connection(True)) as db_conn, closing(
            db_conn.cursor(dictionary=True)
        ) as db_cursor:
            db_cursor.execute(
                f"""
                DELETE FROM `{table_prefix}archives` WHERE
                begin_timestamp >= {begin_ts} AND end_timestamp <= {end_ts}
                RETURNING id
                """
            )
            results = db_cursor.fetchall()

            if 0 == len(results):
                logger.info("No archive meets the deletion condition.")
                return 0

            archives_to_delete = [result["id"] for result in results]
            db_cursor.execute(
                f"""
                DELETE FROM `{table_prefix}files`
                WHERE archive_id in ({', '.join(['%s'] * len(archives_to_delete))})
                """,
                archives_to_delete,
            )
            db_conn.commit()
    except Exception:
        logger.exception("Failed to delete archives from database, abort.")
        return -1

    logger.info(f"Finished deleting archives from the database")

    for archive in archives_to_delete:
        logger.info(f"Deleting archive {archive} from the storage")
        archive_path = archives_dir / archive
        if not archive_path.is_dir():
            logger.warning(f"Archive {archive} is not a directory, skipping deletion")
            continue
        shutil.rmtree(archive_path)

    logger.info(f"Finished deleting archives")
    return 0


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Delete out-of-dated archive.")
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
        help="Time range filter lower-bound (inclusive) as milliseconds" " from the UNIX epoch.",
    )
    args_parser.add_argument(
        "end_ts",
        type=int,
        help="Time range filter lower-bound (inclusive) as milliseconds" " from the UNIX epoch.",
    )
    parsed_args = args_parser.parse_args(argv[1:])

    return handle_file_deletion(
        clp_home,
        pathlib.Path(parsed_args.config),
        default_config_file_path,
        parsed_args.begin_ts,
        parsed_args.end_ts,
    )


if "__main__" == __name__:
    sys.exit(main(sys.argv))
