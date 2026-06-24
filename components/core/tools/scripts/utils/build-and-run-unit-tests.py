#!/usr/bin/env python3
"""Builds the CLP-core's binaries and runs its unit tests."""

from __future__ import annotations

import argparse
import logging
import os
import subprocess
import sys
from pathlib import Path

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

# One hour timeout to prevent changes with infinite loops from running forever
UNIT_TEST_TIMEOUT_SECONDS = 60 * 60

# Minimum memory (in GB) per compilation job.
MIN_MEMORY_PER_JOB_GB = 2


def _get_cgroup_memory_limit_kb() -> int | None:
    """Returns the cgroup memory limit in kB, or None if not available or unlimited."""
    # cgroup v2
    try:
        max_bytes = Path("/sys/fs/cgroup/memory.max").read_text().strip()
        if max_bytes != "max":
            limit_kb = int(max_bytes) // 1024
            if limit_kb > 0:
                return limit_kb
    except (OSError, ValueError):
        pass

    # cgroup v1
    try:
        limit_bytes = Path("/sys/fs/cgroup/memory/memory.limit_in_bytes").read_text().strip()
        limit_int = int(limit_bytes)
        # Some systems use a very large number for "unlimited"
        if 0 < limit_int < 9223372036854771712:
            return limit_int // 1024
    except (OSError, ValueError):
        pass

    return None


def _compute_max_parallel_jobs() -> int:
    """Computes the maximum number of parallel compilation jobs based on CPU count and available
    memory. Each job gets at least MIN_MEMORY_PER_JOB_GB GB of memory, and the result is floored
    at 1.

    Memory detection order:
      1. cgroup v2 memory.max (containers)
      2. cgroup v1 memory.limit_in_bytes (containers)
      3. /proc/meminfo MemTotal (Linux hosts)
      4. Fall back to CPU count with no memory cap
    """
    cpu_count = os.cpu_count() or 1
    mem_limit_jobs = cpu_count

    # Try cgroup limits first (important for containers where /proc/meminfo shows host memory)
    cgroup_kb = _get_cgroup_memory_limit_kb()
    if cgroup_kb is not None:
        mem_limit_jobs = max(1, cgroup_kb // (MIN_MEMORY_PER_JOB_GB * 1024 * 1024))
    else:
        try:
            meminfo = Path("/proc/meminfo").read_text()
            for line in meminfo.splitlines():
                if line.startswith("MemTotal:"):
                    total_kb = int(line.split()[1])
                    mem_limit_jobs = max(1, total_kb // (MIN_MEMORY_PER_JOB_GB * 1024 * 1024))
                    break
        except (OSError, ValueError):
            # /proc/meminfo unavailable (e.g., macOS); fall back to CPU count
            pass

    return max(1, min(cpu_count, mem_limit_jobs))


def _positive_int(value: str) -> int:
    """Argparse type that rejects non-positive integers.

    Only invoked when --num-jobs is given a value, so the ``None`` default (which
    falls back to :func:`_compute_max_parallel_jobs`) is unaffected.
    """
    ivalue = int(value)
    if ivalue < 1:
        raise argparse.ArgumentTypeError(f"{value} is not a positive integer (must be >= 1)")
    return ivalue


def _config_cmake_project(src_dir: Path, build_dir: Path, use_shared_libs: bool) -> None:
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


def _build_project(build_dir: Path, num_jobs: int) -> None:
    """
    :param build_dir:
    :param num_jobs: Max number of jobs to run when building.
    """
    cmd = [
        "cmake",
        "--build",
        str(build_dir),
        "--parallel",
        str(num_jobs),
    ]
    subprocess.run(cmd, check=True)


def _run_unit_tests(build_dir: Path, test_spec: str | None) -> None:
    """
    :param build_dir:
    :param test_spec: Catch2 test specification.
    """
    cmd = [
        "./unitTest",
    ]
    if test_spec is not None:
        cmd.append(test_spec)
    subprocess.run(cmd, cwd=build_dir, check=True, timeout=UNIT_TEST_TIMEOUT_SECONDS)


def main(argv: list[str]) -> int:
    """Builds the CLP-core's binaries and runs its unit tests."""
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
        "--num-jobs",
        type=_positive_int,
        default=None,
        help="Max number of jobs to run when building. Defaults to min(nproc, available_memory_GB"
        f" / {MIN_MEMORY_PER_JOB_GB}) with a floor of 1.",
    )
    args_parser.add_argument("--test-spec", help="Catch2 test specification.")

    parsed_args = args_parser.parse_args(argv[1:])
    src_dir: Path = Path(parsed_args.source_dir)
    build_dir: Path = Path(parsed_args.build_dir)
    use_shared_libs: bool = parsed_args.use_shared_libs
    num_jobs: int = parsed_args.num_jobs if parsed_args.num_jobs is not None else _compute_max_parallel_jobs()
    test_spec: str | None = parsed_args.test_spec

    _config_cmake_project(src_dir, build_dir, use_shared_libs)
    _build_project(build_dir, num_jobs)
    _run_unit_tests(build_dir, test_spec)

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
