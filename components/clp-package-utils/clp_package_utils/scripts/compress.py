import argparse
import configparser
import logging
import pathlib
import subprocess
import sys
import uuid
from typing import List, Tuple

from clp_py_utils.clp_config import CLPConfig, StorageEngine
from job_orchestration.scheduler.job_config import InputType

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    CONTAINER_INPUT_LOGS_ROOT_DIR,
    dump_container_config,
    generate_container_config,
    generate_container_name,
    generate_container_start_cmd,
    get_clp_home,
    JobType,
    load_config_file,
    validate_and_load_db_credentials_file,
)

logger = logging.getLogger(__file__)


def _parse_aws_credentials_file(credentials_file_path: pathlib.Path, user: str) -> Tuple[str, str]:
    """
    Parses the `aws_access_key_id` and `aws_secret_access_key` of `user` from the given
    credentials_file_path.
    :param credentials_file_path:
    :param user:
    :return: A tuple of (aws_access_key_id, aws_secret_access_key)
    :raises: ValueError if the file doesn't exist, or doesn't contain valid aws credentials.
    """

    if not credentials_file_path.exists():
        raise ValueError(f"'{credentials_file_path}' doesn't exist.")

    config_reader = configparser.ConfigParser()
    config_reader.read(credentials_file_path)

    if not config_reader.has_section(user):
        raise ValueError(f"User '{user}' doesn't exist.")

    user_credentials = config_reader[user]
    if "aws_session_token" in user_credentials:
        raise ValueError(f"Session tokens (short-term credentials) are not supported.")

    aws_access_key_id = user_credentials.get("aws_access_key_id")
    aws_secret_access_key = user_credentials.get("aws_secret_access_key")

    if aws_access_key_id is None or aws_secret_access_key is None:
        raise ValueError(
            "The credentials file must contain both aws_access_key_id and aws_secret_access_key."
        )

    return aws_access_key_id, aws_secret_access_key


def _generate_logs_list(
    container_logs_list_path: pathlib.Path,
    parsed_args: argparse.Namespace,
) -> None:
    input_type = parsed_args.input_type

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
            container_logs_list_file.write(f"{parsed_args.url}\n")

    else:
        raise ValueError(f"Unsupported input type: {input_type}.")


def _generate_compress_cmd(
    parsed_args: argparse.Namespace, config_path: pathlib.Path, logs_list_path: pathlib.Path
) -> List[str]:
    input_type = parsed_args.input_type

    # fmt: off
    compress_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.compress",
        "--config", str(config_path),
        input_type,
    ]
    # fmt: on
    if parsed_args.timestamp_key is not None:
        compress_cmd.append("--timestamp-key")
        compress_cmd.append(parsed_args.timestamp_key)
    if parsed_args.tags is not None:
        compress_cmd.append("--tags")
        compress_cmd.append(parsed_args.tags)
    if parsed_args.no_progress_reporting is True:
        compress_cmd.append("--no-progress-reporting")

    if InputType.FS == input_type:
        pass
    elif InputType.S3 == input_type:
        aws_access_key_id = parsed_args.aws_access_key_id
        aws_secret_access_key = parsed_args.aws_secret_access_key
        if parsed_args.aws_credentials_file:
            default_credentials_user = "default"
            aws_access_key_id, aws_secret_access_key = _parse_aws_credentials_file(
                pathlib.Path(parsed_args.aws_credentials_file), default_credentials_user
            )
        if bool(aws_access_key_id) and bool(aws_secret_access_key):
            compress_cmd.append("--aws-access-key-id")
            compress_cmd.append(aws_access_key_id)
            compress_cmd.append("--aws-secret-access-key")
            compress_cmd.append(aws_secret_access_key)
    else:
        raise ValueError(f"Unsupported input type: {input_type}.")

    compress_cmd.append("--logs-list")
    compress_cmd.append(str(logs_list_path))

    return compress_cmd


def _add_common_arguments(args_parser: argparse.ArgumentParser) -> None:
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
    parsed_args: argparse.Namespace, args_parser: argparse.ArgumentParser, clp_config: CLPConfig
) -> None:
    if StorageEngine.CLP_S != clp_config.package.storage_engine:
        args_parser.error(
            f"Input type {InputType.S3} is only supported for the storage engine"
            f" {StorageEngine.CLP_S}."
        )

    # Validate aws credentials were specified using only one method
    aws_credential_file = parsed_args.aws_credentials_file
    aws_access_key_id = parsed_args.aws_access_key_id
    aws_secret_access_key = parsed_args.aws_secret_access_key
    if aws_credential_file is not None:
        if not pathlib.Path(aws_credential_file).exists():
            args_parser.error(f"AWS credentials file '{aws_credential_file}' doesn't exist.")

        if aws_access_key_id is not None or aws_secret_access_key is not None:
            args_parser.error(
                "aws_credentials_file cannot be specified together with aws_access_key_id or"
                " aws_secret_access_key."
            )

    else:
        if not bool(aws_access_key_id):
            args_parser.error("aws_access_key_id not specified or empty")
        if not bool(aws_secret_access_key):
            args_parser.error("aws_secret_access_key not specified or empty")


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
    input_type_args_parser = args_parser.add_subparsers(dest="input_type")

    fs_compressor_parser = input_type_args_parser.add_parser(InputType.FS)
    _add_common_arguments(fs_compressor_parser)
    fs_compressor_parser.add_argument("paths", metavar="PATH", nargs="*", help="Paths to compress.")
    fs_compressor_parser.add_argument(
        "-f", "--path-list", dest="path_list", help="A file listing all paths to compress."
    )

    s3_compressor_parser = input_type_args_parser.add_parser(InputType.S3)
    _add_common_arguments(s3_compressor_parser)
    s3_compressor_parser.add_argument("url", metavar="URL", help="URL of objects to be compressed")
    s3_compressor_parser.add_argument(
        "--aws-access-key-id", type=str, default=None, help="AWS access key ID."
    )
    s3_compressor_parser.add_argument(
        "--aws-secret-access-key", type=str, default=None, help="AWS secret access key."
    )
    s3_compressor_parser.add_argument(
        "--aws-credentials-file", type=str, default=None, help="Path to AWS credentials file."
    )

    parsed_args = args_parser.parse_args(argv[1:])

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

    input_type = parsed_args.input_type
    if InputType.FS == input_type:
        _validate_fs_input_args(parsed_args, args_parser)
    elif InputType.S3 == input_type:
        _validate_s3_input_args(parsed_args, args_parser, clp_config)
    else:
        raise ValueError(f"Unsupported input type: {input_type}.")

    container_name = generate_container_name(str(JobType.COMPRESSION))

    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, container_name
    )

    necessary_mounts = [mounts.clp_home, mounts.input_logs_dir, mounts.data_dir, mounts.logs_dir]

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

    _generate_logs_list(container_logs_list_path, parsed_args)

    container_start_cmd = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.execution_container
    )
    compress_cmd = _generate_compress_cmd(
        parsed_args, generated_config_path_on_container, logs_list_path_on_container
    )
    cmd = container_start_cmd + compress_cmd
    subprocess.run(cmd, check=True)

    # Remove generated files
    generated_config_path_on_host.unlink()

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
