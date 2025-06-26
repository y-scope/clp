import argparse
import logging
import os
import subprocess
import sys
from pathlib import Path
from typing import List, Optional

# Set up console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter(
    "%(asctime)s.%(msecs)03d %(levelname)s [%(module)s] %(message)s", datefmt="%Y-%m-%dT%H:%M:%S"
)
logging_console_handler.setFormatter(logging_formatter)

# Set up root logger
root_logger = logging.getLogger()
root_logger.setLevel(logging.INFO)
root_logger.addHandler(logging_console_handler)

# Create logger
logger = logging.getLogger(__name__)


def _run_unit_tests(build_dir: Path, test_spec: Optional[str]):
    """
    :param build_dir:
    :param test_spec: Catch2 test specification.
    """

    cmd = ["./unitTest"]
    if test_spec is not None:
        cmd.append(test_spec)
    subprocess.run(cmd, cwd=build_dir, check=True)


def main(argv: List[str]) -> int:
    args_parser = argparse.ArgumentParser(
        description="Builds the CLP-core's binaries and runs its unit tests."
    )
    args_parser.add_argument("--build-dir", required=True, help="Build output directory.")
    args_parser.add_argument(
        "--use-shared-libs",
        action="store_true",
        help="Build targets by linking against shared libraries.",
    )
    args_parser.add_argument(
        "--num-jobs", type=int, default=os.cpu_count(), help="Max number of jobs to run when building."
    )
    args_parser.add_argument("--test-spec", help="Catch2 test specification.")

    parsed_args = args_parser.parse_args(argv[1:])
    build_dir: Path = Path(parsed_args.build_dir)
    test_spec: Optional[str] = parsed_args.test_spec

    build_cmd = [
        "python3", "build.py",
        "--build-dir", str(build_dir),
        "--num-jobs", str(parsed_args.num_jobs)
    ]
    if parsed_args.use_shared_libs:
        build_cmd.append("--use-shared-libs")

    logger.info("Starting build process...")
    subprocess.run(build_cmd, check=True)
    logger.info("Build completed. Running unit tests...")
    _run_unit_tests(build_dir, test_spec)
    logger.info("All unit tests completed successfully.")
    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
