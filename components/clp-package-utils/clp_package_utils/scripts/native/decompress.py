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
from clp_py_utils.clp_config import CLP_METADATA_TABLE_PREFIX, CLPConfig, Database
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.scheduler.constants import QueryJobStatus, QueryJobType
from job_orchestration.scheduler.job_config import ExtractIrJobConfig

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    DECOMPRESSION_COMMAND,
    get_clp_home,
    IR_EXTRACTION_COMMAND,
    validate_and_load_config_file,
)
from clp_package_utils.scripts.native.utils import (
    run_function_in_process,
    submit_query_job,
    wait_for_query_job,
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


def get_orig_file_id(db_config: Database, path: str) -> Optional[str]:
    """
    Returns the original file id of the file with the given path
    If multiple files have the same path, this method returns
    the first file returned from the archive metadata database
    :param db_config: config of the archive metadata database
    :param path: original path of the file
    :return: orig_file_id. None if no file matches with input path
    """
    sql_adapter = SQL_Adapter(db_config)
    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        db_cursor.execute(
            f"SELECT orig_file_id FROM `{CLP_METADATA_TABLE_PREFIX}files` WHERE path = (%s)",
            (path,),
        )
        results = db_cursor.fetchall()
        db_conn.commit()

        if len(results) == 0:
            logger.error("No file found for the given file path.")
            return None

        if len(results) > 1:
            logger.warning(
                "Multiple files found for the given file path, "
                "return the orig_file_id of the first file"
            )

        return results[0]["orig_file_id"]


def create_and_monitor_ir_extraction_job_in_db(
    db_config: Database,
    orig_file_id: str,
    msg_ix: int,
    target_size: int | None,
):
    extract_ir_config = ExtractIrJobConfig(
        orig_file_id=orig_file_id,
        msg_ix=msg_ix,
        target_size=target_size,
    )

    sql_adapter = SQL_Adapter(db_config)
    job_id = submit_query_job(sql_adapter, extract_ir_config, QueryJobType.EXTRACT_IR)
    job_status = wait_for_query_job(sql_adapter, job_id)

    if QueryJobStatus.SUCCEEDED == job_status:
        logger.info(f"Finished job {job_id}")
    else:
        logger.error(f"job {job_id} finished with unexpected status: {job_status}")


async def do_extract(
    db_config: Database,
    orig_file_id: str,
    msg_ix: int,
    target_size: int | None,
):
    await run_function_in_process(
        create_and_monitor_ir_extraction_job_in_db, db_config, orig_file_id, msg_ix, target_size
    )


def handle_ir_extraction(
    parsed_args,
    clp_config: CLPConfig,
):
    orig_file_id: str
    if parsed_args.orig_file_id:
        orig_file_id = parsed_args.orig_file_id
    else:
        orig_file_id = get_orig_file_id(clp_config.database, parsed_args.path)
    try:
        asyncio.run(
            do_extract(
                clp_config.database,
                orig_file_id,
                parsed_args.msg_ix,
                parsed_args.target_size,
            )
        )
    except asyncio.CancelledError:
        logger.error("Extraction cancelled.")
        return -1


def handle_decompression(
    parsed_args,
    clp_home: pathlib.Path,
    clp_config: CLPConfig,
):
    # Validate paths were specified using only one method
    if len(parsed_args.paths) > 0 and parsed_args.files_from is not None:
        logger.error("Paths cannot be specified both on the command line and through a file.")

    # Validate extraction directory
    extraction_dir = pathlib.Path(parsed_args.extraction_dir)
    if not extraction_dir.is_dir():
        logger.error(f"extraction-dir ({extraction_dir}) is not a valid directory.")
        return -1

    logs_dir = clp_config.logs_directory
    archives_dir = clp_config.archive_output.directory

    # Generate database config file for clp
    db_config_file_path = logs_dir / f".decompress-db-config-{uuid.uuid4()}.yml"
    with open(db_config_file_path, "w") as f:
        yaml.safe_dump(clp_config.database.get_clp_connection_params_and_type(True), f)

    # fmt: off
    decompression_cmd = [
        str(clp_home / "bin" / "clp"),
        "x", str(archives_dir), str(extraction_dir),
        "--db-config-file", str(db_config_file_path),
    ]
    # fmt: on
    paths = parsed_args.paths
    list_path = parsed_args.files_from
    files_to_decompress_list_path = None
    if list_path is not None:
        decompression_cmd.append("-f")
        decompression_cmd.append(str(list_path))
    elif len(paths) > 0:
        # Write paths to file
        files_to_decompress_list_path = logs_dir / f"paths-to-decompress-{uuid.uuid4()}.txt"
        with open(files_to_decompress_list_path, "w") as stream:
            for path in paths:
                stream.write(path + "\n")

        decompression_cmd.append("-f")
        decompression_cmd.append(str(files_to_decompress_list_path))

    proc = subprocess.Popen(decompression_cmd)
    return_code = proc.wait()
    if 0 != return_code:
        logger.error(f"Decompression failed, return_code={return_code}")
        return return_code

    # Remove generated files
    if files_to_decompress_list_path is not None:
        files_to_decompress_list_path.unlink()
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
    # Decompression command parser
    decompression_job_parser = command_args_parser.add_parser(DECOMPRESSION_COMMAND)
    decompression_job_parser.add_argument(
        "paths", metavar="PATH", nargs="*", help="Files to decompress."
    )
    decompression_job_parser.add_argument(
        "-f", "--files-from", help="A file listing all files to decompress."
    )
    decompression_job_parser.add_argument(
        "-d", "--extraction-dir", metavar="DIR", default=".", help="Decompress files into DIR"
    )
    # IR extraction command parser
    ir_extraction_parser = command_args_parser.add_parser(IR_EXTRACTION_COMMAND)
    ir_extraction_parser.add_argument("msg_ix", type=int, help="Message index.")
    ir_extraction_parser.add_argument("--target-size", type=int, help="Target IR size.")

    group = ir_extraction_parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--orig-file-id", type=str, help="Original file ID.")
    group.add_argument("--path", type=str, help="Path to the file.")

    parsed_args = args_parser.parse_args(argv[1:])

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = validate_and_load_config_file(
            config_file_path, default_config_file_path, clp_home
        )
        clp_config.validate_archive_output_dir()
        clp_config.validate_logs_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    command = parsed_args.command
    if DECOMPRESSION_COMMAND == command:
        return handle_decompression(parsed_args, clp_home, clp_config)
    elif IR_EXTRACTION_COMMAND == command:
        return handle_ir_extraction(parsed_args, clp_config)
    else:
        logger.exception(f"Unexpected command: {command}")
        return -1


if "__main__" == __name__:
    sys.exit(main(sys.argv))
