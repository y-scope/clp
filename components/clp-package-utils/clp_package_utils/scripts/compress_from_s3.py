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
from clp_py_utils.core import resolve_host_path_in_container

from clp_package_utils.general import (
    dump_container_config,
    generate_container_config,
    generate_container_name,
    generate_container_start_cmd,
    get_clp_home,
    get_container_config_filename,
    JobType,
    load_config_file,
    S3_KEY_PREFIX_COMPRESSION,
    S3_OBJECT_COMPRESSION,
    validate_and_load_db_credentials_file,
    validate_dataset_name,
)

logger = logging.getLogger(__name__)


def _generate_url_list(
    subcommand: str,
    container_url_list_path: pathlib.Path,
    parsed_args: argparse.Namespace,
) -> bool:
    """
    Generates URL list file for the native compression script.

    :param subcommand: S3 compression subcommand. Must be `S3_OBJECT_COMPRESSION` or
        `S3_KEY_PREFIX_COMPRESSION`.
    :param container_url_list_path: Path to write URL list.
    :param parsed_args: Parsed command-line arguments.
    :return: Whether any URLs were written to the file.
    """
    with open(container_url_list_path, "w") as url_list_file:
        url_list_file.write(f"{subcommand}\n")

        if parsed_args.inputs_from is None:
            url_list_file.writelines(f"{url}\n" for url in parsed_args.inputs)
            return len(parsed_args.inputs) != 0

        no_url_found = True
        resolved_inputs_from_path = resolve_host_path_in_container(
            pathlib.Path(parsed_args.inputs_from)
        )
        with open(resolved_inputs_from_path, "r") as input_file:
            for line in input_file:
                stripped_url = line.strip()
                if "" == stripped_url:
                    continue
                no_url_found = False
                url_list_file.write(f"{stripped_url}\n")
        return not no_url_found


def _generate_compress_cmd(
    parsed_args: argparse.Namespace,
    dataset: str | None,
    config_path: pathlib.Path,
    url_list_path: pathlib.Path,
) -> list[str]:
    """
    Generates command to run the native compression script.

    :param parsed_args:
    :param dataset:
    :param config_path: Path to the config file (in the container).
    :param url_list_path: Path to the URL list file (in the container).
    :return: The generated command.
    """
    # fmt: off
    compress_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.compress",
        "--config", str(config_path),
        "--input-type", "s3",
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
    compress_cmd.append(str(url_list_path))

    return compress_cmd


def _validate_s3_object_args(
    parsed_args: argparse.Namespace,
    args_parser: argparse.ArgumentParser,
) -> None:
    """
    Validates `S3_OBJECT_COMPRESSION` subcommand arguments.

    :param parsed_args:
    :param args_parser:
    """
    if len(parsed_args.inputs) == 0 and parsed_args.inputs_from is None:
        args_parser.error("No URLs specified.")

    # Validate URLs were specified using only one method
    if len(parsed_args.inputs) > 0 and parsed_args.inputs_from is not None:
        args_parser.error("URLs cannot be specified on the command line AND through a file.")


def _validate_s3_key_prefix_args(
    parsed_args: argparse.Namespace,
    args_parser: argparse.ArgumentParser,
) -> None:
    """
    Validates `S3_KEY_PREFIX_COMPRESSION` subcommand arguments.

    :param parsed_args:
    :param args_parser:
    """
    if parsed_args.inputs_from is None:
        if len(parsed_args.inputs) == 0:
            args_parser.error("No URL specified.")
        if len(parsed_args.inputs) != 1:
            args_parser.error(
                f"{S3_KEY_PREFIX_COMPRESSION} accepts exactly one URL, got {len(parsed_args.inputs)}."
            )

    if len(parsed_args.inputs) > 0 and parsed_args.inputs_from is not None:
        args_parser.error("URL cannot be specified on the command line AND through a file.")


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Compresses logs from S3")

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

    subparsers = args_parser.add_subparsers(dest="subcommand", required=True)

    object_compression_option_parser = subparsers.add_parser(
        S3_OBJECT_COMPRESSION, help="Compress specific S3 objects identified by their full URLs."
    )
    object_compression_option_parser.add_argument(
        "inputs", metavar="URL", nargs="*", help="S3 object URLs."
    )
    object_compression_option_parser.add_argument(
        "--inputs-from", type=str, help="A file containing all S3 object URLs to compress."
    )

    prefix_compression_option_parser = subparsers.add_parser(
        S3_KEY_PREFIX_COMPRESSION, help="Compress all S3 objects under the key prefix."
    )
    prefix_compression_option_parser.add_argument(
        "inputs", metavar="URL", nargs="*", help="S3 prefix URL."
    )
    prefix_compression_option_parser.add_argument(
        "--inputs-from", type=str, help="A file containing S3 key prefix to compress."
    )

    parsed_args = args_parser.parse_args(argv[1:])
    if parsed_args.verbose:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = load_config_file(resolve_host_path_in_container(config_file_path))
        clp_config.validate_logs_dir(True)

        validate_and_load_db_credentials_file(clp_config, clp_home, False)
    except Exception:
        logger.exception("Failed to load config.")
        return -1

    # Validate logs_input type is S3
    if clp_config.logs_input.type != StorageType.S3:
        logger.error(
            "S3 compression expects `logs_input.type` to be `%s`, but `%s` is found. Please update"
            " `clp-config.yaml`.",
            StorageType.S3,
            clp_config.logs_input.type,
        )
        return -1

    storage_engine: StorageEngine = clp_config.package.storage_engine
    dataset = parsed_args.dataset

    if StorageEngine.CLP_S != storage_engine:
        logger.error(
            f"S3 compression requires storage engine {StorageEngine.CLP_S}, but configured engine"
            f" is {storage_engine}."
        )
        return -1

    # TODO: The following dataset validation is duplicated in `compress.py`. We should extract it
    # into a common utility function.
    dataset = CLP_DEFAULT_DATASET_NAME if dataset is None else dataset
    try:
        clp_db_connection_params = clp_config.database.get_clp_connection_params_and_type(True)
        validate_dataset_name(clp_db_connection_params["table_prefix"], dataset)
    except Exception as e:
        logger.error(e)
        return -1

    if parsed_args.timestamp_key is None and not parsed_args.unstructured:
        logger.warning(
            "`--timestamp-key` not specified. Events will not have assigned timestamps and can"
            " only be searched from the command line without a timestamp filter."
        )
    if parsed_args.timestamp_key is not None and parsed_args.unstructured:
        parsed_args.timestamp_key = None
        logger.warning(
            "`--timestamp-key` and `--unstructured` are not compatible. The input logs will be "
            "treated as unstructured, and the argument to `--timestamp-key` will be ignored."
        )

    if parsed_args.subcommand == S3_OBJECT_COMPRESSION:
        _validate_s3_object_args(parsed_args, args_parser)
    else:
        _validate_s3_key_prefix_args(parsed_args, args_parser)

    # TODO: The following container setup code is duplicated in `compress.py`. We should extract it
    # into a common utility function.
    container_name = generate_container_name(str(JobType.COMPRESSION))

    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, get_container_config_filename(container_name)
    )

    necessary_mounts = [mounts.data_dir, mounts.logs_dir]

    while True:
        url_list_filename = f"{uuid.uuid4()}.txt"
        url_list_path_on_host = clp_config.logs_directory / url_list_filename
        resolved_url_list_path_on_host = resolve_host_path_in_container(url_list_path_on_host)
        url_list_path_on_container = container_clp_config.logs_directory / url_list_filename
        if not resolved_url_list_path_on_host.exists():
            break

    if not _generate_url_list(parsed_args.subcommand, resolved_url_list_path_on_host, parsed_args):
        logger.error("No S3 URLs given for compression.")
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
        parsed_args, dataset, generated_config_path_on_container, url_list_path_on_container
    )

    cmd = container_start_cmd + compress_cmd

    proc = subprocess.run(cmd, check=False)
    ret_code = proc.returncode
    if ret_code != 0:
        logger.error("Compression failed.")
        logger.debug(f"Docker command failed: {shlex.join(cmd)}")
    else:
        resolved_url_list_path_on_host.unlink()

    resolved_generated_config_path_on_host = resolve_host_path_in_container(
        generated_config_path_on_host
    )
    resolved_generated_config_path_on_host.unlink()

    return ret_code


if "__main__" == __name__:
    sys.exit(main(sys.argv))
