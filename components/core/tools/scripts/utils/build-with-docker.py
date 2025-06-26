import argparse
import logging
import os
import pathlib
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

# --- Platform Configurations ---
PLATFORM_CONFIGS = {
    "linux/amd64": {
        "build_script": "clp-env-base-manylinux_2_28_x86_64/build.sh",
        "docker_image": "clp-core-dependencies-manylinux_2_28_x86_64"
    },
    "linux/arm64": {
        "build_script": "clp-env-base-manylinux_2_28_aarch64/build.sh",
        "docker_image": "clp-core-dependencies-manylinux_2_28_aarch64"
    }
}


def _build_clp_env_base_image(target_platform: str) -> None:
    """
    Builds the CLP base Docker image for the specified platform.
    """
    docker_images_path = Path(__file__).resolve().parents[2] / "docker-images"
    config = PLATFORM_CONFIGS.get(target_platform)
    if not config:
        raise NotImplementedError(f"Unsupported target platform: {target_platform}")
    build_script_path = docker_images_path / config["build_script"]
    logger.info(f"Building CLP env base image using script: {build_script_path}")
    try:
        subprocess.run([str(build_script_path)], check=True)
    except subprocess.CalledProcessError as e:
        logger.error(f"Failed to build CLP env base image: {e}")
        sys.exit(e.returncode)


def _find_project_root() -> Path:
    root = Path(__file__).resolve()
    for _ in range(PROJECT_ROOT_DEPTH):
        root = root.parent
    return root


def _build_clp_manylinux_2_28_binaries(
        output_path: Path,
        target_platform: str,
        use_shared_libs: bool = False
) -> None:
    """
    Builds the CLP-core binaries for the specified target platform
    inside Docker using manylinux_2_28 based image
    """
    output_path.mkdir(parents=True, exist_ok=True)
    config = PLATFORM_CONFIGS.get(target_platform)
    if not config:
        raise NotImplementedError(f"Unsupported target platform: {target_platform}")

    project_root = _find_project_root()
    build_script = Path("/clp/components/core/tools/scripts/utils/build.py")

    cmd = [
        "docker", "run",
        "--user", f"{os.getuid()}:{os.getgid()}",
        "--platform", target_platform,
        "-v", f"{project_root}:/clp",
        "-v", f"{output_path}:/output",
        f"{config['docker_image']}:dev",
        "python3", str(build_script),
        "--build-dir", "/output"
    ]
    if use_shared_libs:
        cmd.append("--use-shared-libs")

    logger.info("Running Docker build command: %s", " ".join(map(str, cmd)))
    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as e:
        logger.error(f"Docker build failed: {e}")
        sys.exit(e.returncode)


def main(argv: List[str]) -> int:
    args_parser = argparse.ArgumentParser(
        description="Builds the CLP-core's binaries"
    )
    args_parser.add_argument(
        "--output-dir",
        required=True,
        help="Directory to put the compiled CLP-core binaries in."
    )
    args_parser.add_argument(
        "--use-shared-libs",
        action="store_true",
        help="Build targets by linking against shared libraries."
    )
    args_parser.add_argument(
        "--target-platform",
        default="linux/amd64",
        choices=["linux/amd64", "linux/arm64"],
        help="Target platform of the compiled binary: linux/amd64 or linux/arm64"
    )

    parsed_args = args_parser.parse_args(argv[1:])
    output_dir = Path(parsed_args.output_dir).resolve()

    logger.info(f"Target platform: {parsed_args.target_platform}")
    logger.info(f"Output directory: {output_dir}")
    logger.info(f"Use shared libs: {parsed_args.use_shared_libs}")

    _build_clp_env_base_image(parsed_args.target_platform)
    _build_clp_manylinux_2_28_binaries(
        output_dir,
        parsed_args.target_platform,
        parsed_args.use_shared_libs
    )

    logger.info("Build process completed successfully.")
    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
