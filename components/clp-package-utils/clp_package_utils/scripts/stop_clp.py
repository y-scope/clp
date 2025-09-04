import logging
import sys

from clp_py_utils.clp_config import CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

from clp_package_utils.controller import DockerComposeController
from clp_package_utils.general import (
    get_clp_home,
    load_config_file,
)

logger = logging.getLogger(__file__)


def main():
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    try:
        clp_config = load_config_file(default_config_file_path, default_config_file_path, clp_home)
    except:
        logger.exception("Failed to load config.")
        return -1

    try:
        controller = DockerComposeController(clp_config)
        controller.stop()
    except:
        logger.exception("Failed to stop CLP.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main())
