import argparse
import logging
import pathlib
import subprocess
import sys
from typing import Optional

from clp_py_utils.clp_config import CLPConfig, StorageEngine, StorageType

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    DockerMount,
    DockerMountType,
    dump_container_config,
    EXTRACT_FILE_CMD,
    EXTRACT_IR_CMD,
    EXTRACT_JSON_CMD,
    generate_container_config,
    generate_container_name,
    generate_container_start_cmd,
    get_clp_home,
    JobType,
    load_config_file,
    validate_and_load_db_credentials_file,
    validate_path_could_be_dir,
)

logger = logging.getLogger(__file__)


def validate_and_load_config(
    clp_home: pathlib.Path,
    config_file_path: pathlib.Path,
    default_config_file_path: pathlib.Path,
) -> Optional[CLPConfig]:
    """
    Validates and loads the config file.
    :param clp_home:
    :param config_file_path:
    :param default_config_file_path:
    :return: The config object on success, None otherwise.
    """
    try:
        clp_config = load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_logs_dir()

        # Validate and load necessary credentials
        validate_and_load_db_credentials_file(clp_config, clp_home, False)
        return clp_config
    except:
        logger.exception("Failed to load config.")
        return None


def handle_extract_file_cmd(
    parsed_args, clp_home: pathlib.Path, default_config_file_path: pathlib.Path
) -> int:
    """
    Handles the file extraction command.
    :param parsed_args:
    :param clp_home:
    :param default_config_file_path:
    :return: 0 on success, -1 otherwise.
    """
    paths_to_extract_file_path = None
    if parsed_args.files_from:
        paths_to_extract_file_path = pathlib.Path(parsed_args.files_from)

    # Validate extraction directory
    extraction_dir = pathlib.Path(parsed_args.extraction_dir).resolve()
    try:
        validate_path_could_be_dir(extraction_dir)
    except ValueError as ex:
        logger.error(f"extraction-dir is invalid: {ex}")
        return -1

    # Validate and load config file
    clp_config = validate_and_load_config(
        clp_home, pathlib.Path(parsed_args.config), default_config_file_path
    )
    if clp_config is None:
        return -1

    storage_type = clp_config.archive_output.storage.type
    storage_engine = clp_config.package.storage_engine
    if StorageType.FS != storage_type or StorageEngine.CLP != storage_engine:
        logger.error(
            f"File extraction is not supported for archive storage type `{storage_type}` with"
            f" storage engine `{storage_engine}`."
        )
        return -1

    container_name = generate_container_name(str(JobType.FILE_EXTRACTION))
    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, container_name
    )

    # Set up mounts
    extraction_dir.mkdir(exist_ok=True)
    container_extraction_dir = pathlib.Path("/") / "mnt" / "extraction-dir"
    necessary_mounts = [
        mounts.clp_home,
        mounts.data_dir,
        mounts.logs_dir,
        mounts.archives_output_dir,
        DockerMount(DockerMountType.BIND, extraction_dir, container_extraction_dir),
    ]
    container_paths_to_extract_file_path = None
    if paths_to_extract_file_path:
        container_paths_to_extract_file_path = pathlib.Path("/") / "mnt" / "paths-to-extract.txt"
        necessary_mounts.append(
            DockerMount(
                DockerMountType.BIND,
                paths_to_extract_file_path,
                container_paths_to_extract_file_path,
            )
        )
    container_start_cmd = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.execution_container
    )

    # fmt: off
    extract_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.decompress",
        "--config", str(generated_config_path_on_container),
        EXTRACT_FILE_CMD,
        "-d", str(container_extraction_dir),
    ]
    # fmt: on
    for path in parsed_args.paths:
        extract_cmd.append(path)
    if container_paths_to_extract_file_path:
        extract_cmd.append("--input-list")
        extract_cmd.append(container_paths_to_extract_file_path)

    cmd = container_start_cmd + extract_cmd
    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError:
        logger.exception("Docker or file extraction command failed.")
        return -1

    # Remove generated files
    generated_config_path_on_host.unlink()

    return 0


def handle_extract_stream_cmd(
    parsed_args, clp_home: pathlib.Path, default_config_file_path: pathlib.Path
) -> int:
    """
    Handles the stream extraction command.
    :param parsed_args:
    :param clp_home:
    :param default_config_file_path:
    :return: 0 on success, -1 otherwise.
    """
    # Validate and load config file
    clp_config = validate_and_load_config(
        clp_home, pathlib.Path(parsed_args.config), default_config_file_path
    )
    if clp_config is None:
        return -1

    storage_type = clp_config.archive_output.storage.type
    storage_engine = clp_config.package.storage_engine
    if StorageType.S3 == storage_type and StorageEngine.CLP == storage_engine:
        logger.error(
            f"Stream extraction is not supported for archive storage type `{storage_type}` with"
            f" storage engine `{storage_engine}`."
        )
        return -1

    container_name = generate_container_name(str(JobType.IR_EXTRACTION))
    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, container_name
    )
    necessary_mounts = [mounts.clp_home, mounts.logs_dir]
    container_start_cmd = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.execution_container
    )

    # fmt: off
    job_command = parsed_args.command
    extract_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.decompress",
        "--config", str(generated_config_path_on_container),
        job_command
    ]
    # fmt: on

    if EXTRACT_IR_CMD == job_command:
        extract_cmd.append(str(parsed_args.msg_ix))
        if parsed_args.orig_file_id:
            extract_cmd.append("--orig-file-id")
            extract_cmd.append(str(parsed_args.orig_file_id))
        else:
            extract_cmd.append("--orig-file-path")
            extract_cmd.append(str(parsed_args.orig_file_path))
        if parsed_args.target_uncompressed_size:
            extract_cmd.append("--target-uncompressed-size")
            extract_cmd.append(str(parsed_args.target_uncompressed_size))
    elif EXTRACT_JSON_CMD == job_command:
        extract_cmd.append(str(parsed_args.archive_id))
        if parsed_args.target_chunk_size:
            extract_cmd.append("--target-chunk-size")
            extract_cmd.append(str(parsed_args.target_chunk_size))
    else:
        logger.error(f"Unexpected command: {job_command}")
        return -1

    cmd = container_start_cmd + extract_cmd

    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError:
        logger.exception("Docker or stream extraction command failed.")
        return -1

    # Remove generated files
    generated_config_path_on_host.unlink()

    return 0


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Decompresses logs")
    args_parser.add_argument(
        "--config",
        "-c",
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
        "--target-chunk-size",
        type=int,
        help="Target chunk size (B).",
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
