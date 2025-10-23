import argparse
import logging
import pathlib
import sys

from clp_py_utils.clp_config import CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH, OrchestrationType

from clp_package_utils.controller import DockerComposeController, get_or_create_instance_id
from clp_package_utils.general import (
    get_clp_home,
    load_config_file,
    validate_and_load_db_credentials_file,
    validate_and_load_queue_credentials_file,
    validate_and_load_redis_credentials_file,
    validate_and_load_spider_db_credentials_file,
    validate_logs_input_config,
    validate_output_storage_config,
    validate_retention_config,
)

logger = logging.getLogger(__file__)


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Starts CLP")
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

    parsed_args = args_parser.parse_args(argv[1:])

    if parsed_args.verbose:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    try:
        # Validate and load config file.
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = load_config_file(config_file_path, default_config_file_path, clp_home)

        validate_and_load_db_credentials_file(clp_config, clp_home, True)
        validate_and_load_queue_credentials_file(clp_config, clp_home, True)
        validate_and_load_redis_credentials_file(clp_config, clp_home, True)
        if clp_config.compression_scheduler.type == OrchestrationType.spider:
            validate_and_load_spider_db_credentials_file(clp_config, clp_home, True)
        validate_logs_input_config(clp_config)
        validate_output_storage_config(clp_config)
        validate_retention_config(clp_config)

        clp_config.validate_data_dir()
        clp_config.validate_logs_dir()
        clp_config.validate_aws_config_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    try:
        # Create necessary directories.
        clp_config.data_directory.mkdir(parents=True, exist_ok=True)
        clp_config.logs_directory.mkdir(parents=True, exist_ok=True)
        clp_config.archive_output.get_directory().mkdir(parents=True, exist_ok=True)
        clp_config.stream_output.get_directory().mkdir(parents=True, exist_ok=True)
    except:
        logger.exception("Failed to create necessary directories.")
        return -1

    try:
        instance_id = get_or_create_instance_id(clp_config)
        controller = DockerComposeController(clp_config, instance_id)
        controller.start()
    except Exception as ex:
        if type(ex) == ValueError:
            logger.error(f"Failed to start CLP: {ex}")
        else:
            logger.exception("Failed to start CLP.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
