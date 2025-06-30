import argparse
import asyncio
import logging
import pathlib
import subprocess
import sys
import uuid
from contextlib import closing
from typing import Optional

import yaml
from clp_py_utils.clp_config import (
    CLP_DEFAULT_DATASET_NAME,
    CLPConfig,
    Database,
)
from clp_py_utils.clp_metadata_db_utils import get_files_table_name
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.scheduler.constants import QueryJobStatus, QueryJobType
from job_orchestration.scheduler.job_config import (
    ExtractIrJobConfig,
    ExtractJsonJobConfig,
    QueryJobConfig,
)

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    EXTRACT_FILE_CMD,
    EXTRACT_IR_CMD,
    EXTRACT_JSON_CMD,
    get_clp_home,
    load_config_file,
)
from clp_package_utils.scripts.native.utils import (
    run_function_in_process,
    submit_query_job,
    wait_for_query_job,
)

logger = logging.getLogger(__file__)


def get_orig_file_id(db_config: Database, path: str) -> Optional[str]:
    """
    :param db_config:
    :param path: Path of the original file.
    :return: The ID of an original file which has the given path, or None if no such file exists.
    NOTE: Multiple original files may have the same path in which case this method returns the ID of
    only one of them.
    """
    sql_adapter = SQL_Adapter(db_config)
    clp_db_connection_params = db_config.get_clp_connection_params_and_type(True)
    table_prefix = clp_db_connection_params["table_prefix"]
    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        files_table_name = get_files_table_name(table_prefix, None)
        db_cursor.execute(
            f"SELECT orig_file_id FROM `{files_table_name}` WHERE path = (%s)",
            (path,),
        )
        results = db_cursor.fetchall()
        db_conn.commit()

        if len(results) == 0:
            logger.error("No file found for the given path.")
            return None

        if len(results) > 1:
            logger.warning(
                "Multiple files found for the given path."
                " Returning the orig_file_id of one of them."
            )

        return results[0]["orig_file_id"]


def submit_and_monitor_extraction_job_in_db(
    db_config: Database,
    job_type: QueryJobType,
    job_config: QueryJobConfig,
) -> int:
    """
    Submits a stream extraction job to the scheduler and waits until it finishes.
    :param db_config:
    :param job_type:
    :param job_config:
    :return: 0 on success, -1 otherwise.
    """
    sql_adapter = SQL_Adapter(db_config)
    job_id = submit_query_job(sql_adapter, job_config, job_type)
    job_status = wait_for_query_job(sql_adapter, job_id)

    if QueryJobStatus.SUCCEEDED == job_status:
        logger.info(f"Finished extraction job {job_id}.")
        return 0

    logger.error(f"Extraction job {job_id} finished with unexpected status: {job_status.to_str()}.")
    return -1


def handle_extract_stream_cmd(
    parsed_args: argparse.Namespace,
    clp_home: pathlib.Path,
    default_config_file_path: pathlib.Path,
) -> int:
    """
    Handles the stream extraction command.
    :param parsed_args:
    :param clp_home:
    :param default_config_file_path:
    :return: 0 on success, -1 otherwise.
    """
    # Validate and load config file
    clp_config = validate_and_load_config_file(
        clp_home, pathlib.Path(parsed_args.config), default_config_file_path
    )
    if clp_config is None:
        return -1

    command = parsed_args.command

    job_config: QueryJobConfig
    job_type: QueryJobType
    if EXTRACT_IR_CMD == command:
        job_type = QueryJobType.EXTRACT_IR
        orig_file_id: str
        if parsed_args.orig_file_id:
            orig_file_id = parsed_args.orig_file_id
        else:
            orig_file_path = parsed_args.orig_file_path
            orig_file_id = get_orig_file_id(clp_config.database, orig_file_path)
            if orig_file_id is None:
                logger.error(f"Cannot find orig_file_id corresponding to '{orig_file_path}'.")
                return -1
        job_config = ExtractIrJobConfig(
            orig_file_id=orig_file_id,
            msg_ix=parsed_args.msg_ix,
            target_uncompressed_size=parsed_args.target_uncompressed_size,
        )
    elif EXTRACT_JSON_CMD == command:
        job_type = QueryJobType.EXTRACT_JSON
        job_config = ExtractJsonJobConfig(
            dataset=CLP_DEFAULT_DATASET_NAME,
            archive_id=parsed_args.archive_id,
            target_chunk_size=parsed_args.target_chunk_size,
        )
    else:
        logger.error(f"Unsupported stream extraction command: {command}")
        return -1

    try:
        return asyncio.run(
            run_function_in_process(
                submit_and_monitor_extraction_job_in_db,
                clp_config.database,
                job_type,
                job_config,
            )
        )
    except asyncio.CancelledError:
        logger.error("Stream extraction cancelled.")
        return -1


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
        clp_config.validate_archive_output_config()
        clp_config.validate_logs_dir()
        return clp_config
    except Exception:
        logger.exception("Failed to load config.")
        return None


def handle_extract_file_cmd(
    parsed_args: argparse.Namespace, clp_home: pathlib.Path, default_config_file_path: pathlib.Path
) -> int:
    """
    Handles the file extraction command.
    :param parsed_args:
    :param clp_home:
    :param default_config_file_path:
    :return: 0 on success, -1 otherwise.
    """
    # Validate paths were specified using only one method
    if len(parsed_args.paths) > 0 and parsed_args.files_from is not None:
        logger.error("Paths cannot be specified both on the command line and through a file.")
        return -1

    # Validate extraction directory
    extraction_dir = pathlib.Path(parsed_args.extraction_dir)
    if not extraction_dir.is_dir():
        logger.error(f"extraction-dir ({extraction_dir}) is not a valid directory.")
        return -1

    # Validate and load config file
    clp_config = validate_and_load_config_file(
        clp_home, pathlib.Path(parsed_args.config), default_config_file_path
    )
    if clp_config is None:
        return -1

    paths = parsed_args.paths
    list_path = parsed_args.files_from

    logs_dir = clp_config.logs_directory
    archives_dir = clp_config.archive_output.get_directory()

    # Generate database config file for clp
    db_config_file_path = logs_dir / f".decompress-db-config-{uuid.uuid4()}.yml"
    with open(db_config_file_path, "w") as f:
        yaml.safe_dump(clp_config.database.get_clp_connection_params_and_type(True), f)

    # fmt: off
    extract_cmd = [
        str(clp_home / "bin" / "clp"),
        "x", str(archives_dir), str(extraction_dir),
        "--db-config-file", str(db_config_file_path),
    ]
    # fmt: on

    files_to_extract_list_path = None
    if list_path is not None:
        extract_cmd.append("-f")
        extract_cmd.append(str(list_path))
    elif len(paths) > 0:
        # Write paths to file
        files_to_extract_list_path = logs_dir / f"paths-to-extract-{uuid.uuid4()}.txt"
        with open(files_to_extract_list_path, "w") as stream:
            for path in paths:
                stream.write(path + "\n")

        extract_cmd.append("-f")
        extract_cmd.append(str(files_to_extract_list_path))

    proc = subprocess.Popen(extract_cmd)
    return_code = proc.wait()
    if 0 != return_code:
        logger.error(f"File extraction failed, return_code={return_code}")
        return return_code

    # Remove generated files
    if files_to_extract_list_path is not None:
        files_to_extract_list_path.unlink()
    db_config_file_path.unlink()

    return 0


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Decompresses logs.")
    args_parser.add_argument(
        "--config",
        "-c",
        required=True,
        default=str(default_config_file_path),
        help="CLP configuration file.",
    )
    command_args_parser = args_parser.add_subparsers(dest="command", required=True)

    # File extraction command parser
    file_extraction_parser = command_args_parser.add_parser(EXTRACT_FILE_CMD)
    file_extraction_parser.add_argument(
        "paths", metavar="PATH", nargs="*", help="Files to extract."
    )
    file_extraction_parser.add_argument(
        "-f", "--files-from", help="A file listing all files to extract."
    )
    file_extraction_parser.add_argument(
        "-d", "--extraction-dir", metavar="DIR", default=".", help="Extract files into DIR."
    )

    # IR extraction command parser
    ir_extraction_parser = command_args_parser.add_parser(EXTRACT_IR_CMD)
    ir_extraction_parser.add_argument("msg_ix", type=int, help="Message index.")
    ir_extraction_parser.add_argument(
        "--target-uncompressed-size", type=int, help="Target uncompressed IR size."
    )

    group = ir_extraction_parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--orig-file-id", type=str, help="Original file's ID.")
    group.add_argument("--orig-file-path", type=str, help="Original file's path.")

    # JSON extraction command parser
    json_extraction_parser = command_args_parser.add_parser(EXTRACT_JSON_CMD)
    json_extraction_parser.add_argument("archive_id", type=str, help="Archive ID")
    json_extraction_parser.add_argument(
        "--target-chunk-size", type=int, help="Target chunk size (B)."
    )

    parsed_args = args_parser.parse_args(argv[1:])

    command = parsed_args.command
    if EXTRACT_FILE_CMD == command:
        return handle_extract_file_cmd(parsed_args, clp_home, default_config_file_path)
    elif command in (EXTRACT_IR_CMD, EXTRACT_JSON_CMD):
        return handle_extract_stream_cmd(parsed_args, clp_home, default_config_file_path)
    else:
        logger.exception(f"Unexpected command: {command}")
        return -1


if "__main__" == __name__:
    sys.exit(main(sys.argv))
