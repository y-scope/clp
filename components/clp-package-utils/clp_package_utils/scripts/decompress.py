import argparse
import logging
import pathlib
import subprocess
import sys

from clp_py_utils.clp_config import CLPConfig

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    DockerMount,
    DockerMountType,
    dump_container_config,
    generate_container_config,
    generate_container_name,
    generate_container_start_cmd,
    get_clp_home,
    JobType,
    load_config_file,
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
        clp_config = load_config_file(
            config_file_path, default_config_file_path, clp_home
        )
        clp_config.validate_logs_dir()

        # Validate and load necessary credentials
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

    container_name = generate_container_name(JobType.DECOMPRESSION)
    container_clp_config, mounts = generate_container_config(clp_config, clp_home)
    generated_config_path_on_container, generated_config_path_on_host = dump_container_config(
        container_clp_config, clp_config, container_name
    )

    # Set up mounts
    container_extraction_dir = pathlib.Path("/") / "mnt" / "extraction-dir"
    necessary_mounts = [
        mounts.clp_home,
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
    container_start_cmd = generate_container_start_cmd(
        container_name, necessary_mounts, clp_config.execution_container
    )

    # fmt: off
    decompress_cmd = [
        "python3",
        "-m", "clp_package_utils.scripts.native.decompress",
        "--config", str(generated_config_path_on_container),
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
    generated_config_path_on_host.unlink()

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
