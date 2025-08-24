import logging
import subprocess
import sys

logger = logging.getLogger(__file__)


def main():
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
