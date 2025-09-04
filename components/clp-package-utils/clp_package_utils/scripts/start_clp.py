import argparse
import logging
import pathlib
import sys

from clp_py_utils.clp_config import CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

from clp_package_utils.controller import DockerComposeController
from clp_package_utils.general import (
    dump_shared_container_config,
    generate_docker_compose_container_config,
    get_clp_home,
    load_config_file,
    validate_and_load_db_credentials_file,
    validate_and_load_queue_credentials_file,
    validate_and_load_redis_credentials_file,
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

    parsed_args = args_parser.parse_args(argv[1:])

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = load_config_file(config_file_path, default_config_file_path, clp_home)

        validate_and_load_db_credentials_file(clp_config, clp_home, True)
        validate_and_load_queue_credentials_file(clp_config, clp_home, True)
        validate_and_load_redis_credentials_file(clp_config, clp_home, True)
        validate_logs_input_config(clp_config)
        validate_output_storage_config(clp_config)
        validate_retention_config(clp_config)

        clp_config.validate_data_dir()
        clp_config.validate_logs_dir()
        clp_config.validate_aws_config_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    container_clp_config = generate_docker_compose_container_config(clp_config)
    dump_shared_container_config(container_clp_config, clp_config)

    try:
        controller = DockerComposeController(clp_config)
        controller.deploy()
    except Exception as ex:
        if type(ex) == ValueError:
            logger.error(f"Failed to start CLP: {ex}")
        else:
            logger.exception("Failed to start CLP.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
