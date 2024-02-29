import argparse
import logging
import os
import pathlib
import subprocess
import sys
import uuid

import yaml

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    CONTAINER_CLP_HOME,
    CONTAINER_INPUT_LOGS_ROOT_DIR,
    generate_container_config,
    get_clp_home,
    validate_and_load_config_file,
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
        clp_config = validate_and_load_config_file(
            config_file_path, default_config_file_path, clp_home
        )
        clp_config.validate_logs_dir()

        validate_and_load_db_credentials_file(clp_config, clp_home, False)
    except:
        logger.exception("Failed to load config.")
        return -1

    container_name = f"clp-compressor-{str(uuid.uuid4())[-4:]}"

    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    container_config_filename = f".{container_name}-config.yml"
    container_config_file_path_on_host = clp_config.logs_directory / container_config_filename
    with open(container_config_file_path_on_host, "w") as f:
        yaml.safe_dump(container_clp_config.dump_to_primitive_dict(), f)

    clp_site_packages_dir = CONTAINER_CLP_HOME / "lib" / "python3" / "site-packages"
    # fmt: off
    container_start_cmd = [
        "docker", "run",
        "-i",
        "--rm",
        "--network", "host",
        "-w", str(CONTAINER_CLP_HOME),
        "-e", f"PYTHONPATH={clp_site_packages_dir}",
        "-u", f"{os.getuid()}:{os.getgid()}",
        "--name", container_name,
        "--mount", str(mounts.clp_home),
    ]
    # fmt: on
    necessary_mounts = [mounts.input_logs_dir, mounts.data_dir, mounts.logs_dir]
    for mount in necessary_mounts:
        if mount:
            container_start_cmd.append("--mount")
            container_start_cmd.append(str(mount))
    container_start_cmd.append(clp_config.execution_container)

    # fmt: off
    compress_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.compress",
        "--config", str(container_clp_config.logs_directory / container_config_filename),
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
            container_path_list = clp_config.logs_directory / container_path_list_filename
            if not container_path_list.exists():
                break

        with open(parsed_args.path_list, "r") as path_list_file:
            with open(container_path_list, "w") as container_path_list_file:
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
    container_config_file_path_on_host.unlink()

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
