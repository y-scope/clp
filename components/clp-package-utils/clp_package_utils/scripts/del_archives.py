import argparse
import logging
import pathlib
import subprocess
import sys

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

    args_parser = argparse.ArgumentParser(description="Prune the out-dated archives.")
    args_parser.add_argument(
        "--config",
        "-c",
        default=str(default_config_file_path),
        help="CLP package configuration file.",
    )
    args_parser.add_argument(
        "--begin-ts",
        type=int,
        default=0,
        help="Time range filter lower-bound (inclusive) as milliseconds" " from the UNIX epoch.",
    )
    args_parser.add_argument(
        "--end-ts",
        type=int,
        required=True,
        help="Time range filter upper-bound (inclusive) as milliseconds" " from the UNIX epoch.",
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

    # Validate the input timestamp
    begin_ts = parsed_args.begin_ts
    end_ts = parsed_args.end_ts
    if end_ts < 0 or begin_ts < 0:
        logger.error("begin_ts and end_ts must be positive.")
        return -1

    container_name = generate_container_name(JobType.DEL_ARCHIVE)

    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, container_name
    )

    necessary_mounts = [mounts.clp_home, mounts.logs_dir, mounts.archives_output_dir]
    container_start_cmd = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.execution_container
    )

    # fmt: off
    del_archive_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.del_archives",
        "--config", str(generated_config_path_on_container),
        str(begin_ts),
        str(end_ts)

    ]
    # fmt: on

    cmd = container_start_cmd + del_archive_cmd
    subprocess.run(cmd, check=True)

    # Remove generated files
    generated_config_path_on_host.unlink()

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
