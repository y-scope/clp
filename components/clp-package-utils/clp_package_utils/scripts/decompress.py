import argparse
import logging
import pathlib
import shlex
import subprocess
import sys

from clp_py_utils.clp_config import (
    CLP_DB_PASS_ENV_VAR_NAME,
    CLP_DB_USER_ENV_VAR_NAME,
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    CLP_DEFAULT_DATASET_NAME,
    ClpConfig,
    ClpDbUserType,
    StorageEngine,
    StorageType,
)
from clp_py_utils.core import resolve_host_path, resolve_host_path_in_container

from clp_package_utils.general import (
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
    get_container_config_filename,
    JobType,
    load_config_file,
    validate_and_load_db_credentials_file,
    validate_dataset_name,
    validate_path_could_be_dir,
)

logger = logging.getLogger(__file__)


def validate_and_load_config(
    clp_home: pathlib.Path,
    config_file_path: pathlib.Path,
    default_config_file_path: pathlib.Path,
) -> ClpConfig | None:
    """
    Validates and loads the config file.
    :param clp_home:
    :param config_file_path:
    :param default_config_file_path:
    :return: The config object on success, None otherwise.
    """
    try:
        clp_config = load_config_file(resolve_host_path_in_container(config_file_path))
        clp_config.validate_logs_dir(True)

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
    :return: exit code of extraction command, or -1 if an error is encountered.
    """
    paths_to_extract_file_path = None
    if parsed_args.files_from:
        paths_to_extract_file_path = pathlib.Path(parsed_args.files_from)

    # Validate extraction directory
    extraction_dir = pathlib.Path(parsed_args.extraction_dir)
    extraction_dir = resolve_host_path(extraction_dir)
    resolved_extraction_dir = resolve_host_path_in_container(extraction_dir)
    try:
        validate_path_could_be_dir(resolved_extraction_dir)
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
    if StorageType.FS != storage_type:
        logger.error(
            f"File extraction is not supported for archive storage type `{storage_type}` with"
            f" storage engine `{storage_engine}`."
        )
        return -1

    container_name = generate_container_name(str(JobType.FILE_EXTRACTION))
    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, get_container_config_filename(container_name)
    )

    # Set up mounts
    resolved_extraction_dir.mkdir(exist_ok=True)
    container_extraction_dir = pathlib.Path("/") / "mnt" / "extraction-dir"
    necessary_mounts = [
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

    credentials = clp_config.database.credentials
    extra_env_vars = {
        CLP_DB_PASS_ENV_VAR_NAME: credentials[ClpDbUserType.CLP].password,
        CLP_DB_USER_ENV_VAR_NAME: credentials[ClpDbUserType.CLP].username,
    }
    container_start_cmd = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.container_image_ref, extra_env_vars
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

    if parsed_args.verbose:
        extract_cmd.append("--verbose")

    if StorageEngine.CLP == storage_engine:
        # Use either file list or explicit paths; prohibit --dataset flag
        if parsed_args.dataset is not None:
            logger.error(
                f"The --dataset flag cannot be used with the {storage_engine} storage engine."
            )
            # Remove generated files
            generated_config_path_on_host.unlink()
            return -1
        for path in parsed_args.paths:
            extract_cmd.append(path)
        if container_paths_to_extract_file_path:
            extract_cmd.append("--files-from")
            extract_cmd.append(str(container_paths_to_extract_file_path))
    elif StorageEngine.CLP_S == storage_engine:
        # Prohibit both file list and explicit paths
        if parsed_args.files_from or parsed_args.paths:
            logger.error(
                "File paths cannot be specified when decompressing with the"
                f" {storage_engine} storage engine."
            )
            # Remove generated files
            generated_config_path_on_host.unlink()
            return -1

        dataset = parsed_args.dataset or CLP_DEFAULT_DATASET_NAME
        try:
            clp_db_connection_params = clp_config.database.get_clp_connection_params_and_type(True)
            validate_dataset_name(clp_db_connection_params["table_prefix"], dataset)
        except Exception as e:
            logger.error(e)
            # Remove generated files
            generated_config_path_on_host.unlink()
            return -1

        extract_cmd.extend(["--dataset", dataset])
    else:
        logger.error(f"Unsupported storage engine: {storage_engine}")
        # Remove generated files
        generated_config_path_on_host.unlink()
        return -1

    cmd = container_start_cmd + extract_cmd

    proc = subprocess.run(cmd, check=False)
    ret_code = proc.returncode
    if 0 != ret_code:
        logger.error("File extraction failed.")
        logger.debug(f"Docker command failed: {shlex.join(cmd)}")
    else:
        logger.info(f"File extraction successful. Decompressed file written to {extraction_dir!s}")

    # Remove generated files
    resolved_generated_config_path_on_host = resolve_host_path_in_container(
        generated_config_path_on_host
    )
    resolved_generated_config_path_on_host.unlink()

    return ret_code


def handle_extract_stream_cmd(
    parsed_args, clp_home: pathlib.Path, default_config_file_path: pathlib.Path
) -> int:
    """
    Handles the stream extraction command.
    :param parsed_args:
    :param clp_home:
    :param default_config_file_path:
    :return: exit code of extraction command, or -1 if an error is encountered.
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

    job_command = parsed_args.command
    if EXTRACT_IR_CMD == job_command and StorageEngine.CLP != storage_engine:
        logger.error(f"IR extraction is not supported for storage engine `{storage_engine}`.")
        return -1
    if EXTRACT_JSON_CMD == job_command and StorageEngine.CLP_S != storage_engine:
        logger.error(f"JSON extraction is not supported for storage engine `{storage_engine}`.")
        return -1

    container_name = generate_container_name(str(JobType.IR_EXTRACTION))
    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, get_container_config_filename(container_name)
    )
    necessary_mounts = [mounts.logs_dir]
    credentials = clp_config.database.credentials
    extra_env_vars = {
        CLP_DB_PASS_ENV_VAR_NAME: credentials[ClpDbUserType.CLP].password,
        CLP_DB_USER_ENV_VAR_NAME: credentials[ClpDbUserType.CLP].username,
    }
    container_start_cmd = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.container_image_ref, extra_env_vars
    )

    # fmt: off
    extract_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.decompress",
        "--config", str(generated_config_path_on_container),
        job_command
    ]
    # fmt: on
    if parsed_args.verbose:
        extract_cmd.append("--verbose")

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
        dataset = parsed_args.dataset
        dataset = CLP_DEFAULT_DATASET_NAME if dataset is None else dataset
        try:
            clp_db_connection_params = clp_config.database.get_clp_connection_params_and_type(True)
            validate_dataset_name(clp_db_connection_params["table_prefix"], dataset)
        except Exception as e:
            logger.error(e)
            return -1

        extract_cmd.append(str(parsed_args.archive_id))
        if dataset is not None:
            extract_cmd.append("--dataset")
            extract_cmd.append(dataset)
        if parsed_args.target_chunk_size:
            extract_cmd.append("--target-chunk-size")
            extract_cmd.append(str(parsed_args.target_chunk_size))
    else:
        logger.error(f"Unexpected command: {job_command}")
        return -1

    cmd = container_start_cmd + extract_cmd

    proc = subprocess.run(cmd, check=False)
    ret_code = proc.returncode
    if 0 != ret_code:
        logger.error("Stream extraction failed.")
        logger.debug(f"Docker command failed: {shlex.join(cmd)}")

    # Remove generated files
    resolved_generated_config_path_on_host = resolve_host_path_in_container(
        generated_config_path_on_host
    )
    resolved_generated_config_path_on_host.unlink()

    return ret_code


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
    args_parser.add_argument(
        "--verbose",
        "-v",
        action="store_true",
        help="Enable debug logging.",
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
        "-d",
        "--extraction-dir",
        metavar="DIR",
        default=".",
        help="Extract files into DIR.",
    )
    file_extraction_parser.add_argument(
        "--dataset",
        type=str,
        default=None,
        help="Dataset to decompress (required for clp-json; invalid for clp-text).",
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
        "--dataset",
        type=str,
        default=None,
        help="The dataset that the archives belong to.",
    )
    json_extraction_parser.add_argument(
        "--target-chunk-size",
        type=int,
        help="Target chunk size (B).",
    )

    parsed_args = args_parser.parse_args(argv[1:])
    if parsed_args.verbose:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    command = parsed_args.command
    if EXTRACT_FILE_CMD == command:
        return handle_extract_file_cmd(parsed_args, clp_home, default_config_file_path)
    if command in (EXTRACT_IR_CMD, EXTRACT_JSON_CMD):
        return handle_extract_stream_cmd(parsed_args, clp_home, default_config_file_path)
    logger.exception(f"Unexpected command: {command}")
    return -1


if "__main__" == __name__:
    sys.exit(main(sys.argv))
