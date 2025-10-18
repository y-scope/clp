import argparse
import logging
import pathlib
import shlex
import subprocess
import sys
import uuid
from typing import List, Optional

from clp_py_utils.clp_config import (
    CLP_DB_PASS_ENV_VAR_NAME,
    CLP_DB_USER_ENV_VAR_NAME,
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    CLP_DEFAULT_DATASET_NAME,
    StorageEngine,
)
from job_orchestration.scheduler.job_config import InputType

from clp_package_utils.general import (
    CONTAINER_INPUT_LOGS_ROOT_DIR,
    dump_container_config,
    generate_container_config,
    generate_container_name,
    generate_container_start_cmd,
    get_clp_home,
    get_container_config_filename,
    JobType,
    load_config_file,
    validate_and_load_db_credentials_file,
    validate_dataset_name,
)

logger = logging.getLogger(__file__)


def _generate_logs_list(
    input_type: InputType,
    container_logs_list_path: pathlib.Path,
    parsed_args: argparse.Namespace,
) -> None:
    if InputType.FS == input_type:
        host_logs_list_path = parsed_args.path_list
        with open(container_logs_list_path, "w") as container_logs_list_file:
            if host_logs_list_path is not None:
                with open(host_logs_list_path, "r") as host_logs_list_file:
                    for line in host_logs_list_file:
                        stripped_path_str = line.rstrip()
                        if "" == stripped_path_str:
                            # Skip empty paths
                            continue
                        resolved_path = pathlib.Path(stripped_path_str).resolve()
                        mounted_path = CONTAINER_INPUT_LOGS_ROOT_DIR / resolved_path.relative_to(
                            resolved_path.anchor
                        )
                        container_logs_list_file.write(f"{mounted_path}\n")

            for path in parsed_args.paths:
                resolved_path = pathlib.Path(path).resolve()
                mounted_path = CONTAINER_INPUT_LOGS_ROOT_DIR / resolved_path.relative_to(
                    resolved_path.anchor
                )
                container_logs_list_file.write(f"{mounted_path}\n")

    elif InputType.S3 == input_type:
        with open(container_logs_list_path, "w") as container_logs_list_file:
            container_logs_list_file.write(f"{parsed_args.paths[0]}\n")

    else:
        raise ValueError(f"Unsupported input type: {input_type}.")


def _generate_compress_cmd(
    parsed_args: argparse.Namespace,
    dataset: Optional[str],
    config_path: pathlib.Path,
    logs_list_path: pathlib.Path,
) -> List[str]:

    # fmt: off
    compress_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.compress",
        "--config", str(config_path),
    ]
    # fmt: on
    if parsed_args.verbose:
        compress_cmd.append("--verbose")
    if dataset is not None:
        compress_cmd.append("--dataset")
        compress_cmd.append(dataset)
    if parsed_args.timestamp_key is not None:
        compress_cmd.append("--timestamp-key")
        compress_cmd.append(parsed_args.timestamp_key)
    if parsed_args.tags is not None:
        compress_cmd.append("--tags")
        compress_cmd.append(parsed_args.tags)
    if parsed_args.no_progress_reporting is True:
        compress_cmd.append("--no-progress-reporting")

    compress_cmd.append("--logs-list")
    compress_cmd.append(str(logs_list_path))

    return compress_cmd


def _validate_fs_input_args(
    parsed_args: argparse.Namespace,
    args_parser: argparse.ArgumentParser,
) -> None:
    # Validate some input paths were specified
    if len(parsed_args.paths) == 0 and parsed_args.path_list is None:
        args_parser.error("No paths specified.")

    # Validate paths were specified using only one method
    if len(parsed_args.paths) > 0 and parsed_args.path_list is not None:
        args_parser.error("Paths cannot be specified on the command line AND through a file.")


def _validate_s3_input_args(
    parsed_args: argparse.Namespace,
    args_parser: argparse.ArgumentParser,
    storage_engine: StorageEngine,
) -> None:
    if StorageEngine.CLP_S != storage_engine:
        args_parser.error(
            f"Input type {InputType.S3} is only supported for the storage engine"
            f" {StorageEngine.CLP_S}."
        )
    if len(parsed_args.paths) != 1:
        args_parser.error(f"Only one URL can be specified for input type {InputType.S3}.")
    if parsed_args.path_list is not None:
        args_parser.error(f"Path list file is unsupported for input type {InputType.S3}.")


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Compresses logs")

    # Package-level config option
    args_parser.add_argument(
        "--config",
        "-c",
        default=str(default_config_file_path),
        help="CLP package configuration file.",
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
    args_parser.add_argument(
        "--timestamp-key",
        help="The path (e.g. x.y) for the field containing the log event's timestamp.",
    )
    args_parser.add_argument(
        "-t", "--tags", help="A comma-separated list of tags to apply to the compressed archives."
    )
    args_parser.add_argument(
        "--no-progress-reporting", action="store_true", help="Disables progress reporting."
    )
    args_parser.add_argument(
        "paths", metavar="PATH", nargs="*", help="Paths or an S3 URL to compress."
    )
    args_parser.add_argument(
        "-f", "--path-list", dest="path_list", help="A file listing all paths to compress."
    )

    parsed_args = args_parser.parse_args(argv[1:])
    if parsed_args.verbose:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_logs_dir()

        # Validate and load necessary credentials
        validate_and_load_db_credentials_file(clp_config, clp_home, False)
    except:
        logger.exception("Failed to load config.")
        return -1

    storage_engine: StorageEngine = clp_config.package.storage_engine
    dataset = parsed_args.dataset
    if StorageEngine.CLP_S == storage_engine:
        dataset = CLP_DEFAULT_DATASET_NAME if dataset is None else dataset
        try:
            clp_db_connection_params = clp_config.database.get_clp_connection_params_and_type(True)
            validate_dataset_name(clp_db_connection_params["table_prefix"], dataset)
        except Exception as e:
            logger.error(e)
            return -1

        if parsed_args.timestamp_key is None:
            logger.warning(
                "`--timestamp-key` not specified. Events will not have assigned timestamps and can "
                "only be searched from the command line without a timestamp filter."
            )
    elif dataset is not None:
        logger.error(f"Dataset selection is not supported for storage engine: {storage_engine}.")
        return -1

    input_type = clp_config.logs_input.type
    if InputType.FS == input_type:
        _validate_fs_input_args(parsed_args, args_parser)
    elif InputType.S3 == input_type:
        _validate_s3_input_args(parsed_args, args_parser, storage_engine)
    else:
        raise ValueError(f"Unsupported input type: {input_type}.")

    container_name = generate_container_name(str(JobType.COMPRESSION))

    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, get_container_config_filename(container_name)
    )

    necessary_mounts = [mounts.clp_home, mounts.data_dir, mounts.logs_dir]
    if InputType.FS == input_type:
        necessary_mounts.append(mounts.input_logs_dir)

    # Write compression logs to a file
    while True:
        # Get unused output path
        container_logs_list_filename = f"{uuid.uuid4()}.txt"
        container_logs_list_path = clp_config.logs_directory / container_logs_list_filename
        logs_list_path_on_container = (
            container_clp_config.logs_directory / container_logs_list_filename
        )
        if not container_logs_list_path.exists():
            break

    _generate_logs_list(clp_config.logs_input.type, container_logs_list_path, parsed_args)

    extra_env_vars = {
        CLP_DB_USER_ENV_VAR_NAME: clp_config.database.username,
        CLP_DB_PASS_ENV_VAR_NAME: clp_config.database.password,
    }
    container_start_cmd = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.container_image_ref, extra_env_vars
    )
    compress_cmd = _generate_compress_cmd(
        parsed_args, dataset, generated_config_path_on_container, logs_list_path_on_container
    )

    cmd = container_start_cmd + compress_cmd

    proc = subprocess.run(cmd)
    ret_code = proc.returncode
    if 0 != ret_code:
        logger.error("Compression failed.")
        logger.debug(f"Docker command failed: {shlex.join(cmd)}")

    # Remove generated files
    generated_config_path_on_host.unlink()

    return ret_code


if "__main__" == __name__:
    sys.exit(main(sys.argv))
