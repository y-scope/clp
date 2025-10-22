import argparse
import logging
import pathlib
import shlex
import subprocess
import sys
import uuid
from typing import List

from clp_py_utils.clp_config import (
    CLP_DB_PASS_ENV_VAR_NAME,
    CLP_DB_USER_ENV_VAR_NAME,
    CLP_DEFAULT_DATASET_NAME,
    StorageEngine,
)

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
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


def _generate_url_list(
    subcommand: str,
    container_url_list_path: pathlib.Path,
    parsed_args: argparse.Namespace,
) -> None:
    """
    Generates URL list file for native script.

    :param subcommand: 's3-object' or 's3-key-prefix'
    :param container_url_list_path: Path to write URL list
    :param parsed_args: Parsed command-line arguments
    """
    with open(container_url_list_path, "w") as url_list_file:
        url_list_file.write(f"{subcommand}\n")

        if parsed_args.inputs_from is not None:
            with open(parsed_args.inputs_from, "r") as input_file:
                for line in input_file:
                    stripped_url = line.strip()
                    if "" == stripped_url:
                        continue
                    url_list_file.write(f"{stripped_url}\n")
        else:
            for url in parsed_args.inputs:
                url_list_file.write(f"{url}\n")


def _generate_compress_cmd(
    parsed_args: argparse.Namespace,
    dataset: str,
    config_path: pathlib.Path,
    url_list_path: pathlib.Path,
) -> List[str]:
    """
    Generates command to run native compress script.

    :param parsed_args: Parsed arguments
    :param dataset: Dataset name
    :param config_path: Path to config file (in container)
    :param url_list_path: Path to URL list file (in container)
    :return: Command list
    """
    # fmt: off
    compress_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.compress",
        "--config", str(config_path),
        "--source", "s3",
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
    compress_cmd.append(str(url_list_path))

    return compress_cmd


def _validate_s3_object_args(
    parsed_args: argparse.Namespace,
    args_parser: argparse.ArgumentParser,
) -> None:
    """
    Validates s3-object subcommand arguments.
    """
    if len(parsed_args.inputs) == 0 and parsed_args.inputs_from is None:
        args_parser.error("No paths specified.")

    # Validate paths were specified using only one method
    if len(parsed_args.inputs) > 0 and parsed_args.inputs_from is not None:
        args_parser.error("Paths cannot be specified on the command line AND through a file.")


def _validate_s3_key_prefix_args(
    parsed_args: argparse.Namespace,
    args_parser: argparse.ArgumentParser,
) -> None:
    """
    Validates s3-key-prefix subcommand arguments.
    """

    if parsed_args.inputs_from is None:
        if len(parsed_args.inputs) == 0:
            args_parser.error("No URL specified.")
        if len(parsed_args.inputs) != 1:
            args_parser.error(
                f"s3-key-prefix accepts exactly one URL, got {len(parsed_args.inputs)}."
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
        "-t", "--tags", help="A comma-separated list of tags to apply to the compressed archives."
    )
    args_parser.add_argument(
        "--no-progress-reporting", action="store_true", help="Disables progress reporting."
    )

    subparsers = args_parser.add_subparsers(dest="subcommand", required=True)

    object_parser = subparsers.add_parser("s3-object", help="Compress specific S3 objects")
    object_parser.add_argument("inputs", metavar="URL", nargs="*", help="S3 object URLs")
    object_parser.add_argument(
        "--inputs-from", type=str, help="File containing S3 object URLs (one per line)"
    )

    prefix_parser = subparsers.add_parser(
        "s3-key-prefix", help="Compress all objects under S3 key prefix"
    )
    prefix_parser.add_argument("inputs", metavar="URL", nargs="*", help="S3 prefix URL")
    prefix_parser.add_argument(
        "--inputs-from", type=str, help="File containing S3 prefix URL"
    )

    parsed_args = args_parser.parse_args(argv[1:])
    if parsed_args.verbose:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = load_config_file(config_file_path, default_config_file_path, clp_home)
        clp_config.validate_logs_dir()

        validate_and_load_db_credentials_file(clp_config, clp_home, False)
    except Exception:
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
    else:
        logger.error(
            f"S3 compression requires storage engine {StorageEngine.CLP_S}, "
            f"but configured engine is {storage_engine}."
        )
        return -1

    if parsed_args.subcommand == "s3-object":
        _validate_s3_object_args(parsed_args, args_parser)
    else:
        _validate_s3_key_prefix_args(parsed_args, args_parser)

    container_name = generate_container_name(str(JobType.COMPRESSION))

    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, get_container_config_filename(container_name)
    )

    necessary_mounts = [mounts.clp_home, mounts.data_dir, mounts.logs_dir]

    while True:
        container_url_list_filename = f"{uuid.uuid4()}.txt"
        container_url_list_path = clp_config.logs_directory / container_url_list_filename
        url_list_path_on_container = (
            container_clp_config.logs_directory / container_url_list_filename
        )
        if not container_url_list_path.exists():
            break

    _generate_url_list(parsed_args.subcommand, container_url_list_path, parsed_args)

    extra_env_vars = {
        CLP_DB_USER_ENV_VAR_NAME: clp_config.database.username,
        CLP_DB_PASS_ENV_VAR_NAME: clp_config.database.password,
    }
    container_start_cmd = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.container_image_ref, extra_env_vars
    )
    compress_cmd = _generate_compress_cmd(
        parsed_args, dataset, generated_config_path_on_container, url_list_path_on_container
    )

    cmd = container_start_cmd + compress_cmd

    proc = subprocess.run(cmd)
    ret_code = proc.returncode
    if 0 != ret_code:
        logger.error("Compression failed.")
        logger.debug(f"Docker command failed: {shlex.join(cmd)}")

    generated_config_path_on_host.unlink()

    return ret_code


if "__main__" == __name__:
    sys.exit(main(sys.argv))
