#!/usr/bin/env python3

"""Script to start multiple Spider workers."""

import argparse
import logging
import os
import pathlib
import signal
import subprocess
import sys
import threading

# Setup logging
# Create logger
logger = logging.getLogger("start-spider-worker")
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)

SPIDER_WORKER_TERM_TIMEOUT_SECONDS = 5

stop_event = threading.Event()


def handle_sigterm(signum, frame) -> None:
    """
    Signal handler for SIGTERM.
    Stops all running workers gracefully.

    :param signum: The signal number.
    :param frame: The current stack frame.
    """
    logger.info("Received SIGTERM, stopping workers...")
    stop_event.set()


def parse_args() -> argparse.Namespace:
    """
    Parses command line arguments.
    :return: The parsed arguments.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--num-workers", type=int, default=1, help="Number of concurrent Spider workers."
    )
    parser.add_argument("--storage-url", type=str, required=True, help="Spider storage URL.")
    parser.add_argument("--host", type=str, required=True, help="The worker host.")
    return parser.parse_args()


def main() -> None:
    """Main function to start multiple Spider workers."""
    # Parse arguments
    args = parse_args()
    num_workers = args.num_workers
    if num_workers < 1:
        logger.error("Number of concurrent workers must be at least 1.")
        sys.exit(1)
    storage_url = args.storage_url
    host = args.host

    clp_home = os.getenv("CLP_HOME", "/opt/clp")
    spider_worker_path = pathlib.Path(clp_home) / "bin" / "spider_worker"
    if not spider_worker_path.exists():
        logger.error("spider_worker not found at %s.", spider_worker_path)
        sys.exit(1)

    # Start multiple spider workers
    processes = []
    try:
        for i in range(num_workers):
            proc = subprocess.Popen(
                [spider_worker_path, "--storage_url", storage_url, "--host", host]
            )
            processes.append(proc)
            logger.info("Started Spider worker %d/%d (PID: %d).", i + 1, num_workers, proc.pid)
    except OSError:
        logger.exception("Failed to start Spider worker.")
        for proc in processes:
            logger.info("Terminating worker process %d.", proc.pid)
            proc.terminate()
        wait_exit(processes)
        sys.exit(1)

    signal.signal(signal.SIGTERM, handle_sigterm)

    # Wait for SIGTERM
    stop_event.wait()

    # Wait for termination with timeout
    exit_code = wait_exit(processes)
    sys.exit(exit_code)


def wait_exit(processes: list[subprocess.Popen]) -> int:
    """
    Waits for all processes to exit.
    If the process does not exit within the timeout, it is killed.

    :param processes: List of subprocess.Popen objects.
    :return: Exit code of the first non-zero exit code, or 0 if all processes exited successfully.
    """
    exit_code = 0
    for proc in processes:
        try:
            code = proc.wait(timeout=SPIDER_WORKER_TERM_TIMEOUT_SECONDS)
            if code != 0 and exit_code == 0:
                exit_code = code
        except subprocess.TimeoutExpired:
            logger.warning(
                "Worker process %d did not terminate in time, sending SIGKILL.", proc.pid
            )
            proc.kill()
            proc.wait()
    return exit_code


if __name__ == "__main__":
    main()
