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

# Number of parent directories to reach project root from this script
PROJECT_ROOT_DEPTH = 6


def _run_subprocess(cmd: List[str], cwd: Optional[Path] = None) -> None:
    logger.info(f"Running: {' '.join(cmd)} in {cwd if cwd else Path.cwd()}")
    try:
        subprocess.run(cmd, check=True, cwd=str(cwd) if cwd else None)
    except subprocess.CalledProcessError as e:
        logger.error(f"Command failed: {' '.join(cmd)} (in {cwd})\nReturn code: {e.returncode}")
        sys.exit(e.returncode)


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
    subprocess.run(cmd, check=True, cwd=str(build_dir))


def _find_project_root() -> Path:
    root = Path(__file__).resolve()
    for _ in range(PROJECT_ROOT_DEPTH):
        root = root.parent
    return root


def main(argv: List[str]) -> int:
    args_parser = argparse.ArgumentParser(
        description="Builds standard CLP-core binaries in the current local environment"
    )
    args_parser.add_argument("--build-dir", required=True, help="Build output directory.")
    args_parser.add_argument(
        "--use-shared-libs",
        action="store_true",
        help="Build targets by linking against shared libraries.",
    )
    args_parser.add_argument(
        "--num-jobs",
        type=int,
        default=os.cpu_count(),
        help="Max number of jobs to run when building.",
    )

    parsed_args = args_parser.parse_args(argv[1:])
    project_root = _find_project_root()
    logger.info(f"Inferred project root: {project_root}")

    # Initialize core dependencies
    logger.info("Cleaning project...")
    _run_subprocess(["task", "clean"], cwd=project_root)
    logger.info("Installing core dependencies...")
    _run_subprocess(["task", "deps:core"], cwd=project_root)

    # Create build directory
    build_path = Path(parsed_args.build_dir)
    if not build_path.exists():
        logger.info(f"Creating build directory: {build_path}")
        build_path.mkdir(parents=True, exist_ok=True)

    # Configure and build core
    core_path = project_root / "components" / "core"
    logger.info(f"Configuring CMake project in {core_path} -> {build_path}")
    _config_cmake_project(core_path, build_path, parsed_args.use_shared_libs)

    logger.info(f"Building project in {build_path} with {parsed_args.num_jobs} jobs")
    _build_project(build_path, parsed_args.num_jobs)

    logger.info("Build completed successfully.")
    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
