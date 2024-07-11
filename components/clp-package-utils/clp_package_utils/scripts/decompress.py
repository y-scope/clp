import argparse
import logging
import pathlib
import subprocess
import sys
from typing import Optional

from clp_py_utils.clp_config import CLPConfig

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    DECOMPRESSION_COMMAND,
    DockerMount,
    DockerMountType,
    dump_container_config,
    generate_container_config,
    generate_container_name,
    generate_container_start_cmd,
    get_clp_home,
    IR_EXTRACTION_COMMAND,
    JobType,
    load_config_file,
    validate_and_load_db_credentials_file,
    validate_path_could_be_dir,
)

# Setup logging
# Create logger
logger = logging.getLogger("clp")
logger.setLevel(logging.DEBUG)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] [%(name)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


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
    :return: clp_config on success, None otherwise.
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


def handle_decompression_command(
    parsed_args, clp_home: pathlib.Path, default_config_file_path: pathlib.Path
):
    paths_to_decompress_file_path = None
    if parsed_args.files_from:
        paths_to_decompress_file_path = pathlib.Path(parsed_args.files_from)

    # Validate extraction directory
    extraction_dir = pathlib.Path(parsed_args.extraction_dir).resolve()
    try:
        validate_path_could_be_dir(extraction_dir)
    except ValueError as ex:
        logger.error(f"extraction-dir is invalid: {ex}")
        return -1
    extraction_dir.mkdir(exist_ok=True)

    # Validate and load config file
    clp_config = validate_and_load_config(
        clp_home, pathlib.Path(parsed_args.config), default_config_file_path
    )
    if not clp_config:
        return -1

    container_name = generate_container_name(JobType.DECOMPRESSION)
    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, container_name
    )

    # Set up mounts
    container_extraction_dir = pathlib.Path("/") / "mnt" / "extraction-dir"
    necessary_mounts = [
        mounts.clp_home,
        mounts.data_dir,
        mounts.logs_dir,
        mounts.archives_output_dir,
        DockerMount(DockerMountType.BIND, extraction_dir, container_extraction_dir),
    ]
    container_paths_to_decompress_file_path = None
    if paths_to_decompress_file_path:
        container_paths_to_decompress_file_path = (
            pathlib.Path("/") / "mnt" / "paths-to-decompress.txt"
        )
        necessary_mounts.append(
            DockerMount(
                DockerMountType.BIND,
                paths_to_decompress_file_path,
                container_paths_to_decompress_file_path,
            )
        )
    container_start_cmd = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.execution_container
    )

    # fmt: off
    decompress_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.decompress",
        "--config", str(generated_config_path_on_container),
        DECOMPRESSION_COMMAND,
        "-d", str(container_extraction_dir),
    ]
    # fmt: on
    for path in parsed_args.paths:
        decompress_cmd.append(path)
    if container_paths_to_decompress_file_path:
        decompress_cmd.append("--input-list")
        decompress_cmd.append(container_paths_to_decompress_file_path)

    cmd = container_start_cmd + decompress_cmd
    subprocess.run(cmd, check=True)

    # Remove generated files
    generated_config_path_on_host.unlink()


def handle_extraction(parsed_args, clp_home: pathlib.Path, default_config_file_path: pathlib.Path):
    # Validate and load config file
    clp_config = validate_and_load_config(
        clp_home, pathlib.Path(parsed_args.config), default_config_file_path
    )
    if not clp_config:
        return -1

    container_name = generate_container_name(JobType.IR_EXTRACTION)
    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, container_name
    )
    necessary_mounts = [mounts.clp_home, mounts.logs_dir]
    container_start_cmd = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.execution_container
    )

    extract_cmd = [
        "python3",
        "-m",
        "clp_package_utils.scripts.native.decompress",
        "--config",
        str(generated_config_path_on_container),
        IR_EXTRACTION_COMMAND,
        str(parsed_args.msg_ix),
    ]
    # fmt: on
    if parsed_args.orig_file_id:
        extract_cmd.append("--orig-file-id")
        extract_cmd.append(str(parsed_args.orig_file_id))
    else:
        extract_cmd.append("--path")
        extract_cmd.append(str(parsed_args.path))
    if parsed_args.target_uncompressed_size:
        extract_cmd.append("--target-uncompressed-size")
        extract_cmd.append(str(parsed_args.target_uncompressed_size))
    cmd = container_start_cmd + extract_cmd
    subprocess.run(cmd, check=True)

    # Remove generated files
    generated_config_path_on_host.unlink()


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Decompresses logs")
    args_parser.add_argument(
        "--config",
        "-c",
        type=str,
        default=str(default_config_file_path),
        help="CLP package configuration file.",
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
    ir_extraction_parser.add_argument(
        "--target-uncompressed-size", type=int, help="Target uncompressed IR size."
    )

    group = ir_extraction_parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--orig-file-id", type=str, help="Original file ID.")
    group.add_argument("--path", type=str, help="Path to the file.")

    parsed_args = args_parser.parse_args(argv[1:])

    command = parsed_args.command
    if DECOMPRESSION_COMMAND == command:
        return handle_decompression_command(parsed_args, clp_home, default_config_file_path)
    elif IR_EXTRACTION_COMMAND == command:
        return handle_extraction(parsed_args, clp_home, default_config_file_path)
    else:
        logger.exception(f"Unexpected command: {command}")
        return -1


if "__main__" == __name__:
    sys.exit(main(sys.argv))
