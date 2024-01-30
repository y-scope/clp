import argparse
import logging
import os
import pathlib
import subprocess
import sys
import uuid

import yaml

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    CONTAINER_CLP_HOME,
    DockerMount,
    DockerMountType,
    generate_container_config,
    get_clp_home,
    validate_and_load_config_file,
    validate_and_load_db_credentials_file,
    validate_path_could_be_dir,
)

# Setup logging
# Create logger
logger = logging.getLogger("clp")
logger.setLevel(logging.DEBUG)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] [%(name)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Decompresses logs")
    args_parser.add_argument(
        "--config",
        "-c",
        type=str,
        default=str(default_config_file_path),
        help="CLP package configuration file.",
    )
    args_parser.add_argument("paths", metavar="PATH", nargs="*", help="Files to decompress.")
    args_parser.add_argument("-f", "--files-from", help="A file listing all files to decompress.")
    args_parser.add_argument(
        "-d", "--extraction-dir", metavar="DIR", default=".", help="Decompress files into DIR"
    )
    parsed_args = args_parser.parse_args(argv[1:])

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = validate_and_load_config_file(
            config_file_path, default_config_file_path, clp_home
        )
        clp_config.validate_logs_dir()

        validate_and_load_db_credentials_file(clp_config, clp_home, False)
    except:
        logger.exception("Failed to load config.")
        return -1

    paths_to_decompress_file_path = None
    if parsed_args.files_from:
        paths_to_decompress_file_path = pathlib.Path(parsed_args.files_from)

    # Validate extraction directory
    extraction_dir = pathlib.Path(parsed_args.extraction_dir).resolve()
    try:
        validate_path_could_be_dir(extraction_dir)
    except ValueError as ex:
        logger.error(f"extraction-dir is invalid: {ex}")
        return -1
    extraction_dir.mkdir(exist_ok=True)

    container_name = f"clp-decompressor-{str(uuid.uuid4())[-4:]}"

    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    container_config_filename = f".{container_name}-config.yml"
    container_config_file_path_on_host = clp_config.logs_directory / container_config_filename
    with open(container_config_file_path_on_host, "w") as f:
        yaml.safe_dump(container_clp_config.dump_to_primitive_dict(), f)

    clp_site_packages_dir = CONTAINER_CLP_HOME / "lib" / "python3" / "site-packages"
    # fmt: off
    container_start_cmd = [
        "docker", "run",
        "-i",
        "--rm",
        "--network", "host",
        "-w", str(CONTAINER_CLP_HOME),
        "-e", f"PYTHONPATH={clp_site_packages_dir}",
        "-u", f"{os.getuid()}:{os.getgid()}",
        "--name", container_name,
        "--mount", str(mounts.clp_home),
    ]
    # fmt: on

    # Set up mounts
    container_extraction_dir = pathlib.Path("/") / "mnt" / "extraction-dir"
    necessary_mounts = [
        mounts.data_dir,
        mounts.logs_dir,
        mounts.archives_output_dir,
        DockerMount(DockerMountType.BIND, extraction_dir, container_extraction_dir),
    ]
    container_paths_to_decompress_file_path = None
    if paths_to_decompress_file_path:
        container_paths_to_decompress_file_path = (
            pathlib.Path("/") / "mnt" / "paths-to-decompress.txt"
        )
        necessary_mounts.append(
            DockerMount(
                DockerMountType.BIND,
                paths_to_decompress_file_path,
                container_paths_to_decompress_file_path,
            )
        )
    for mount in necessary_mounts:
        if mount:
            container_start_cmd.append("--mount")
            container_start_cmd.append(str(mount))

    container_start_cmd.append(clp_config.execution_container)

    # fmt: off
    decompress_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.decompress",
        "--config", str(container_clp_config.logs_directory / container_config_filename),
        "-d", str(container_extraction_dir),
    ]
    # fmt: on
    for path in parsed_args.paths:
        decompress_cmd.append(path)
    if container_paths_to_decompress_file_path:
        decompress_cmd.append("--input-list")
        decompress_cmd.append(container_paths_to_decompress_file_path)

    cmd = container_start_cmd + decompress_cmd
    subprocess.run(cmd, check=True)

    # Remove generated files
    container_config_file_path_on_host.unlink()

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
