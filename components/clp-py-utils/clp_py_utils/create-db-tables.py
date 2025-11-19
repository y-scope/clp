import argparse
import logging
import pathlib
import subprocess
import sys

from clp_py_utils.clp_config import StorageEngine, ClpConfig
from clp_py_utils.core import read_yaml_config_file

# Setup logging
# Create logger
logger = logging.getLogger(__file__)
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def main(argv):
    args_parser = argparse.ArgumentParser(description="Creates database tables for CLP.")
    args_parser.add_argument("--config", "-c", required=True, help="CLP configuration file.")
    args_parser.add_argument(
        "--storage-engine",
        type=str,
        choices=[engine.value for engine in StorageEngine],
        required=True,
        help="Compression storage engine to use.",
    )
    parsed_args = args_parser.parse_args(argv[1:])

    config_file_path = pathlib.Path(parsed_args.config)
    storage_engine = StorageEngine(parsed_args.storage_engine)

    script_dir = pathlib.Path(__file__).parent.resolve()

    # fmt: off
    cmd = [
        "python3", str(script_dir / "initialize-clp-metadata-db.py"),
        "--config", str(config_file_path),
        "--storage-engine", str(storage_engine),
    ]
    # fmt: on
    subprocess.run(cmd, check=True)

    # fmt: off
    cmd = [
        "python3", str(script_dir / "initialize-orchestration-db.py"),
        "--config", str(config_file_path),
    ]
    # fmt: on
    subprocess.run(cmd, check=True)

    try:
        clp_config = ClpConfig.model_validate(read_yaml_config_file(pathlib.Path(config_file_path)))
        clp_config.database.load_credentials_from_env()
        if clp_config.spider_db is None:
            logger.info("No spider database configured, skipping spider DB initialization.")
            return 0
    except Exception as e:
        logger.error(f"Failed to load CLP configuration: {e}")
        return 1
    # fmt: off
    cmd = [
        "python3", str(script_dir / "initialize-spider-db.py"),
        "--config", str(config_file_path),
    ]
    # fmt: on
    subprocess.run(cmd, check=True)

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
