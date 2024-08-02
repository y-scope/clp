import argparse
import logging
import pathlib
import subprocess
import sys
import uuid

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

# Setup logging
# Create logger
logger = logging.getLogger(__file__)
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] [%(name)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Compresses files/directories")
    args_parser.add_argument(
        "--config",
        "-c",
        default=str(default_config_file_path),
        help="CLP package configuration file.",
    )
    args_parser.add_argument("paths", metavar="PATH", nargs="*", help="Paths to compress.")
    args_parser.add_argument(
        "-f", "--path-list", dest="path_list", help="A file listing all paths to compress."
    )
    args_parser.add_argument(
        "--timestamp-key",
        help="The path (e.g. x.y) for the field containing the log event's timestamp.",
    )
    args_parser.add_argument(
        "-t", "--tags", help="A comma-separated list of tags to apply to the compressed archives."
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

    container_name = generate_container_name(JobType.COMPRESSION)

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
        "--config", str(generated_config_path_on_container),
        "--remove-path-prefix", str(CONTAINER_INPUT_LOGS_ROOT_DIR),
    ]
    # fmt: on
    if parsed_args.timestamp_key is not None:
        compress_cmd.append("--timestamp-key")
        compress_cmd.append(parsed_args.timestamp_key)
    if parsed_args.tags is not None:
        compress_cmd.append("--tags")
        compress_cmd.append(parsed_args.tags)
    for path in parsed_args.paths:
        # Resolve path and prefix it with CONTAINER_INPUT_LOGS_ROOT_DIR
        resolved_path = pathlib.Path(path).resolve()
        path = str(CONTAINER_INPUT_LOGS_ROOT_DIR / resolved_path.relative_to(resolved_path.anchor))
        compress_cmd.append(path)
    if parsed_args.path_list is not None:
        # Get unused output path
        while True:
            container_path_list_filename = f"{uuid.uuid4()}.txt"
            container_path_list_path = clp_config.logs_directory / container_path_list_filename
            if not container_path_list_path.exists():
                break

        with open(parsed_args.path_list, "r") as path_list_file:
            with open(container_path_list_path, "w") as container_path_list_file:
                for line in path_list_file:
                    resolved_path = pathlib.Path(line.rstrip()).resolve()
                    path = CONTAINER_INPUT_LOGS_ROOT_DIR / resolved_path.relative_to(
                        resolved_path.anchor
                    )
                    container_path_list_file.write(f"{path}\n")

        compress_cmd.append("--path-list")
        compress_cmd.append(container_clp_config.logs_directory / container_path_list_filename)

    cmd = container_start_cmd + compress_cmd
    subprocess.run(cmd, check=True)

    # Remove generated files
    generated_config_path_on_host.unlink()

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
