import argparse
import logging
import pathlib
import subprocess
import sys
import uuid
from typing import List

from clp_py_utils.clp_config import CLPConfig, StorageEngine
from clp_py_utils.s3_utils import parse_aws_credentials_file
from job_orchestration.scheduler.job_config import InputType

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
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


def generate_targets_list(
    input_type: InputType,
    container_targets_list_path: pathlib.Path,
    parsed_args: argparse.Namespace,
) -> None:
    if InputType.FS == input_type:
        compression_targets_list_file = parsed_args.path_list
        with open(container_targets_list_path, "w") as container_targets_list_file:
            if compression_targets_list_file is not None:
                with open(compression_targets_list_file, "r") as targets_list_file:
                    for line in targets_list_file:
                        resolved_path = pathlib.Path(line.rstrip()).resolve()
                        container_targets_list_file.write(f"{resolved_path}\n")

            for path in parsed_args.paths:
                resolved_path = pathlib.Path(path).resolve()
                container_targets_list_file.write(f"{resolved_path}\n")

    elif InputType.S3 == input_type:
        with open(container_targets_list_path, "w") as container_targets_list_file:
            container_targets_list_file.write(f"{parsed_args.url}\n")

    else:
        raise ValueError(f"Unsupported input type: {input_type}.")


def append_input_specific_args(compress_cmd: List[str], parsed_args: argparse.Namespace) -> None:
    input_type = parsed_args.input_type

    if InputType.FS == input_type:
        return
    elif InputType.S3 == input_type:
        aws_access_key_id = parsed_args.aws_access_key_id
        aws_secret_access_key = parsed_args.aws_secret_access_key
        if parsed_args.aws_credentials_file:
            aws_access_key_id, aws_secret_access_key = parse_aws_credentials_file(
                pathlib.Path(parsed_args.aws_credentials_file)
            )
        if aws_access_key_id and aws_secret_access_key:
            compress_cmd.append("--aws-access-key-id")
            compress_cmd.append(aws_access_key_id)
            compress_cmd.append("--aws-secret-access-key")
            compress_cmd.append(aws_secret_access_key)
    else:
        raise ValueError(f"Unsupported input type: {input_type}.")


def add_common_arguments(
    args_parser: argparse.ArgumentParser, default_config_file_path: pathlib.Path
) -> None:
    args_parser.add_argument(
        "--config",
        "-c",
        default=str(default_config_file_path),
        help="CLP package configuration file.",
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


def validate_fs_input_args(
    parsed_args: argparse.Namespace,
    args_parser: argparse.ArgumentParser,
) -> None:
    # Validate some input paths were specified
    if len(parsed_args.paths) == 0 and parsed_args.path_list is None:
        args_parser.error("No paths specified.")

    # Validate paths were specified using only one method
    if len(parsed_args.paths) > 0 and parsed_args.path_list is not None:
        args_parser.error("Paths cannot be specified on the command line AND through a file.")


def validate_s3_input_args(
    parsed_args: argparse.Namespace, args_parser: argparse.ArgumentParser, clp_config: CLPConfig
) -> None:
    if StorageEngine.CLP_S != clp_config.package.storage_engine:
        raise ValueError(
            f"input type {InputType.S3} is only supported with storage engine {StorageEngine.CLP_S}"
        )

    # Validate aws credentials were specified using only one method
    aws_credential_file = parsed_args.aws_credentials_file
    aws_access_key_id = parsed_args.aws_access_key_id
    aws_secret_access_key = parsed_args.aws_secret_access_key
    if aws_credential_file is not None:
        if not pathlib.Path(aws_credential_file).exists():
            raise ValueError(f"{aws_credential_file} doesn't exist.")

        if aws_access_key_id is not None or aws_secret_access_key is not None:
            args_parser.error(
                "aws_credentials_file can not be specified together with aws_access_key_id or aws_secret_access_key."
            )

    elif bool(aws_access_key_id) != bool(aws_secret_access_key):
        args_parser.error(
            "aws_access_key_id and aws_secret_access_key must be both set or left unset."
        )


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Compresses files from filesystem/s3")
    input_type_args_parser = args_parser.add_subparsers(dest="input_type")

    fs_compressor_parser = input_type_args_parser.add_parser(InputType.FS)
    add_common_arguments(fs_compressor_parser, default_config_file_path)
    fs_compressor_parser.add_argument("paths", metavar="PATH", nargs="*", help="Paths to compress.")
    fs_compressor_parser.add_argument(
        "-f", "--path-list", dest="path_list", help="A file listing all paths to compress."
    )

    s3_compressor_parser = input_type_args_parser.add_parser(InputType.S3)
    add_common_arguments(s3_compressor_parser, default_config_file_path)
    s3_compressor_parser.add_argument("url", metavar="URL", help="URL of object to be compressed")
    s3_compressor_parser.add_argument(
        "--aws-access-key-id", type=str, default=None, help="AWS access key id."
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
        validate_fs_input_args(parsed_args, args_parser)
    elif InputType.S3 == input_type:
        validate_s3_input_args(parsed_args, args_parser, clp_config)
    else:
        raise ValueError(f"Unsupported input type: {input_type}.")

    container_name = generate_container_name(str(JobType.COMPRESSION))

    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, container_name
    )

    necessary_mounts = [mounts.clp_home, mounts.input_logs_dir, mounts.data_dir, mounts.logs_dir]
    container_start_cmd = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.execution_container
    )

    # fmt: off
    compress_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.compress",
        input_type,
        "--config", str(generated_config_path_on_container)
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

    append_input_specific_args(compress_cmd, parsed_args)

    # Write targets to compress to a file
    while True:
        # Get unused output path
        container_path_list_filename = f"{uuid.uuid4()}.txt"
        container_path_list_path = clp_config.logs_directory / container_path_list_filename
        if not container_path_list_path.exists():
            break

    generate_targets_list(input_type, container_path_list_path, parsed_args)
    compress_cmd.append("--path-list")
    compress_cmd.append(str(container_clp_config.logs_directory / container_path_list_filename))

    cmd = container_start_cmd + compress_cmd
    subprocess.run(cmd, check=True)

    # Remove generated files
    generated_config_path_on_host.unlink()

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
