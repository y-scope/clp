import argparse
import logging
import pathlib
import shlex
import subprocess
import sys
import uuid

from clp_py_utils.clp_config import (
    CLP_DB_PASS_ENV_VAR_NAME,
    CLP_DB_USER_ENV_VAR_NAME,
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    CLP_DEFAULT_DATASET_NAME,
    ClpDbUserType,
    StorageEngine,
    StorageType,
)
from clp_py_utils.core import resolve_host_path, resolve_host_path_in_container

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

logger = logging.getLogger(__name__)


def _generate_logs_list(
    container_logs_list_path: pathlib.Path,
    parsed_args: argparse.Namespace,
) -> bool:
    """
    Generates logs list file for the native compression script.

    :param container_logs_list_path: Path to write logs list.
    :param parsed_args: Parsed command-line arguments.
    :return: Whether any paths were written to the logs list.
    """
    host_logs_list_path = parsed_args.path_list
    with open(container_logs_list_path, "w") as container_logs_list_file:
        if host_logs_list_path is None:
            for path in parsed_args.paths:
                resolved_path = resolve_host_path(pathlib.Path(path))
                mounted_path = CONTAINER_INPUT_LOGS_ROOT_DIR / resolved_path.relative_to(
                    resolved_path.anchor
                )
                container_logs_list_file.write(f"{mounted_path}\n")
            return len(parsed_args.paths) != 0

        no_path_found = True
        resolved_host_logs_list_path = resolve_host_path_in_container(
            pathlib.Path(host_logs_list_path)
        )
        with open(resolved_host_logs_list_path, "r") as host_logs_list_file:
            for line in host_logs_list_file:
                stripped_path_str = line.rstrip()
                if "" == stripped_path_str:
                    # Skip empty paths
                    continue
                no_path_found = False
                resolved_path = resolve_host_path(pathlib.Path(stripped_path_str))
                mounted_path = CONTAINER_INPUT_LOGS_ROOT_DIR / resolved_path.relative_to(
                    resolved_path.anchor
                )
                container_logs_list_file.write(f"{mounted_path}\n")
        return not no_path_found


def _generate_compress_cmd(
    parsed_args: argparse.Namespace,
    dataset: str | None,
    config_path: pathlib.Path,
    logs_list_path: pathlib.Path,
) -> list[str]:
    # fmt: off
    compress_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.compress",
        "--config", str(config_path),
        "--input-type", "fs",
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
    if parsed_args.unstructured:
        compress_cmd.append("--unstructured")
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
        "--unstructured",
        action="store_true",
        help="Treat all inputs as unstructured text logs.",
    )
    args_parser.add_argument(
        "--no-progress-reporting", action="store_true", help="Disables progress reporting."
    )
    args_parser.add_argument("paths", metavar="PATH", nargs="*", help="Paths to compress.")
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
        clp_config = load_config_file(resolve_host_path_in_container(config_file_path))
        clp_config.validate_logs_dir(True)

        # Validate and load necessary credentials
        validate_and_load_db_credentials_file(clp_config, clp_home, False)
    except Exception:
        logger.exception("Failed to load config.")
        return -1

    # Validate logs_input type is FS
    if clp_config.logs_input.type != StorageType.FS:
        logger.error(
            "Filesystem compression expects `logs_input.type` to be `%s`, but `%s` is found. For S3"
            " compression, use `compress-from-s3.sh` instead.",
            StorageType.FS,
            clp_config.logs_input.type,
        )
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

        if parsed_args.timestamp_key is None and not parsed_args.unstructured:
            logger.warning(
                "`--timestamp-key` not specified. Events will not have assigned timestamps and can "
                "only be searched from the command line without a timestamp filter."
            )
        if parsed_args.timestamp_key is not None and parsed_args.unstructured:
            parsed_args.timestamp_key = None
            logger.warning(
                "`--timestamp-key` and `--unstructured` are not compatible. The input logs will be "
                "treated as unstructured, and the argument to `--timestamp-key` will be ignored."
            )
    elif dataset is not None:
        logger.error(f"Dataset selection is not supported for storage engine: {storage_engine}.")
        return -1

    # Validate filesystem input arguments
    _validate_fs_input_args(parsed_args, args_parser)

    container_name = generate_container_name(str(JobType.COMPRESSION))

    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, get_container_config_filename(container_name)
    )

    necessary_mounts = [mounts.data_dir, mounts.logs_dir, mounts.input_logs_dir]

    # Write compression logs to a file
    while True:
        # Get unused output path
        logs_list_filename = f"{uuid.uuid4()}.txt"
        logs_list_path_on_host = clp_config.logs_directory / logs_list_filename
        resolved_logs_list_path_on_host = resolve_host_path_in_container(logs_list_path_on_host)
        logs_list_path_on_container = container_clp_config.logs_directory / logs_list_filename
        if not resolved_logs_list_path_on_host.exists():
            break

    if not _generate_logs_list(resolved_logs_list_path_on_host, parsed_args):
        logger.error("No filesystem paths given for compression.")
        return -1

    credentials = clp_config.database.credentials
    extra_env_vars = {
        CLP_DB_PASS_ENV_VAR_NAME: credentials[ClpDbUserType.CLP].password,
        CLP_DB_USER_ENV_VAR_NAME: credentials[ClpDbUserType.CLP].username,
    }
    container_start_cmd = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.container_image_ref, extra_env_vars
    )
    compress_cmd = _generate_compress_cmd(
        parsed_args, dataset, generated_config_path_on_container, logs_list_path_on_container
    )

    cmd = container_start_cmd + compress_cmd

    proc = subprocess.run(cmd, check=False)
    ret_code = proc.returncode
    if ret_code != 0:
        logger.error("Compression failed.")
        logger.debug(f"Docker command failed: {shlex.join(cmd)}")
    else:
        resolved_logs_list_path_on_host.unlink()

    resolved_generated_config_path_on_host = resolve_host_path_in_container(
        generated_config_path_on_host
    )
    resolved_generated_config_path_on_host.unlink()

    return ret_code


if "__main__" == __name__:
    sys.exit(main(sys.argv))
