import argparse
import logging
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


def _config_cmake_project(src_dir: Path, build_dir: Path, use_shared_libs: bool):
    cmd = [
        "cmake",
        "-S",
        str(src_dir),
        "-B",
        str(build_dir),
    ]
    if use_shared_libs:
        cmd.append("-DCLP_USE_STATIC_LIBS=OFF")
    subprocess.run(cmd, check=True)


def _build_project(build_dir: Path, num_jobs: Optional[int]):
    """
    :param build_dir:
    :param num_jobs: Max number of jobs to run when building.
    """

    cmd = [
        "cmake",
        "--build",
        str(build_dir),
        "--parallel",
    ]
    if num_jobs is not None:
        cmd.append(str(num_jobs))
    subprocess.run(cmd, check=True)


def _run_unit_tests(build_dir: Path, test_spec: Optional[str]):
    """
    :param build_dir:
    :param test_spec: Catch2 test specification.
    """

    cmd = [
        "./unitTest",
    ]
    if test_spec is not None:
        cmd.append(test_spec)
    subprocess.run(cmd, cwd=build_dir, check=True)


def main(argv: List[str]) -> int:
    args_parser = argparse.ArgumentParser(
        description="Builds the CLP-core's binaries and runs its unit tests."
    )
    args_parser.add_argument(
        "--source-dir", required=True, help="Directory containing the main CMakeLists.txt."
    )
    args_parser.add_argument("--build-dir", required=True, help="Build output directory.")
    args_parser.add_argument(
        "--use-shared-libs",
        action="store_true",
        help="Build targets by linking against shared libraries.",
    )
    args_parser.add_argument(
        "--num-jobs", type=int, help="Max number of jobs to run when building."
    )
    args_parser.add_argument("--test-spec", help="Catch2 test specification.")

    parsed_args = args_parser.parse_args(argv[1:])
    src_dir: Path = Path(parsed_args.source_dir)
    build_dir: Path = Path(parsed_args.build_dir)
    use_shared_libs: bool = parsed_args.use_shared_libs
    num_jobs: Optional[int] = parsed_args.num_jobs
    test_spec: Optional[str] = parsed_args.test_spec

    _config_cmake_project(src_dir, build_dir, use_shared_libs)
    _build_project(build_dir, num_jobs)
    _run_unit_tests(build_dir, test_spec)

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
