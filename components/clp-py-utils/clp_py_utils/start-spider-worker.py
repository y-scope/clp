import argparse
import logging
import os
import pathlib
import socket
import subprocess

# Setup logging
# Create logger
logger = logging.getLogger("start-spider-worker")
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def parse_args() -> argparse.Namespace:
    """
    Parses command line arguments.
    :return: The parsed arguments.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--concurrency", type=int, default=1, help="Number of concurrent Spider workers."
    )
    parser.add_argument("--storage-url", type=str, required=True, help="Spider storage URL.")
    parser.add_argument("--host", type=str, required=False, default=None, help="The worker host.")
    return parser.parse_args()


def main() -> None:
    """Main function to start multiple spider workers."""
    # Parse arguments
    args = parse_args()
    concurrency = args.concurrency
    if concurrency < 1:
        logger.error("Concurrency must be at least 1.")
        exit(1)
    storage_url = args.storage_url
    host = args.host
    if host is None:
        try:
            host = socket.gethostbyname(socket.gethostname())
        except (socket.gaierror, socket.herror) as e:
            logger.error(f"Failed to resolve hostname: {e}")
            exit(1)

    clp_home = os.getenv("CLP_HOME", "/opt/clp")
    spider_worker_path = pathlib.Path(clp_home) / "bin" / "spider_worker"
    if not spider_worker_path.exists():
        logger.error(f"spider_worker not found at {spider_worker_path}")
        exit(1)

    # Start multiple spider workers
    processes = []
    try:
        for _ in range(concurrency):
            process = subprocess.Popen(
                [spider_worker_path, "--storage_url", storage_url, "--host", host]
            )
            processes.append(process)
    except OSError as e:
        logger.error(f"Failed to start spider worker: {e}")
        for process in processes:
            process.terminate()
        exit(1)

    exit_code = 0
    for process in processes:
        worker_proc_exit_code = process.wait()
        if worker_proc_exit_code != 0:
            logger.error(f"Spider worker exited with code {worker_proc_exit_code}")
            exit_code = 1

    exit(exit_code)


if __name__ == "__main__":
    main()
