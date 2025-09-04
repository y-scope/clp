import logging
import subprocess
import sys

from clp_package_utils.general import check_docker_dependencies

logger = logging.getLogger(__file__)


def main():
    try:
        check_docker_dependencies(should_compose_run=True)
    except:
        logger.exception("Dependency checking failed.")
        return -1

    logger.info("Stopping all CLP containers using Docker Compose...")
    try:
        subprocess.run(
            ["docker", "compose", "down"],
            stderr=subprocess.STDOUT,
            check=True,
        )
        logger.info("All CLP containers stopped.")
    except subprocess.CalledProcessError:
        logger.exception("Failed to stop CLP containers using Docker Compose.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main())
