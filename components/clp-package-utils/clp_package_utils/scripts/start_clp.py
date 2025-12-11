import argparse
import logging
import pathlib
import sys

from clp_py_utils.clp_config import CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH
from clp_py_utils.core import resolve_host_path_in_container

from clp_package_utils.controller import DockerComposeController, get_or_create_instance_id
from clp_package_utils.general import (
    get_clp_home,
    load_config_file,
    validate_and_load_db_credentials_file,
    validate_and_load_queue_credentials_file,
    validate_and_load_redis_credentials_file,
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
    args_parser.add_argument(
        "--setup-only",
        action="store_true",
        help="Validate configuration and prepare directories without starting services.",
    )

    parsed_args = args_parser.parse_args(argv[1:])

    if parsed_args.verbose:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)

    try:
        # Validate and load config file.
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = load_config_file(resolve_host_path_in_container(config_file_path))

        validate_and_load_db_credentials_file(clp_config, clp_home, True)
        if clp_config.queue is not None:
            validate_and_load_queue_credentials_file(clp_config, clp_home, True)
        if clp_config.redis is not None:
            validate_and_load_redis_credentials_file(clp_config, clp_home, True)
        clp_config.validate_logs_input_config(True)
        validate_output_storage_config(clp_config)
        validate_retention_config(clp_config)

        clp_config.validate_api_server()
        clp_config.validate_aws_config_dir(True)
        clp_config.validate_data_dir(True)
        clp_config.validate_logs_dir(True)
        clp_config.validate_tmp_dir(True)
    except:
        logger.exception("Failed to load config.")
        return -1

    try:
        # Create necessary directories.
        resolve_host_path_in_container(clp_config.data_directory).mkdir(parents=True, exist_ok=True)
        resolve_host_path_in_container(clp_config.logs_directory).mkdir(parents=True, exist_ok=True)
        resolve_host_path_in_container(clp_config.tmp_directory).mkdir(parents=True, exist_ok=True)
        resolve_host_path_in_container(clp_config.archive_output.get_directory()).mkdir(
            parents=True, exist_ok=True
        )
        resolve_host_path_in_container(clp_config.stream_output.get_directory()).mkdir(
            parents=True, exist_ok=True
        )
    except:
        logger.exception("Failed to create necessary directories.")
        return -1

    try:
        instance_id = get_or_create_instance_id(clp_config)
        controller = DockerComposeController(clp_config, instance_id)
        controller.set_up_env()
        if parsed_args.setup_only:
            logger.info(
                "Completed setup. Services not started because `--setup-only` was specified."
            )
            return 0
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
