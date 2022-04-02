import argparse
import logging
import pathlib
import subprocess
import sys

# Setup logging
# Create logger
logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter('%(asctime)s [%(levelname)s] %(message)s')
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def main(argv):
    args_parser = argparse.ArgumentParser(description='Create tables for CLP in database.')
    args_parser.add_argument('--config', required=True, help='CLP config file.')
    args_parser.add_argument('--timeout', required=True, type=int, help='Maximum duration (seconds) to wait.')
    parsed_args = args_parser.parse_args(argv[1:])

    config_file_path = pathlib.Path(parsed_args.config)

    script_dir = pathlib.Path(__file__).parent.resolve()

    cmd = [
        'python3', f'{script_dir / "initialize-clp-metadata-db.py"}',
        '--config', str(config_file_path)
    ]
    subprocess.run(cmd, check=True)

    cmd = [
        'python3', f'{script_dir / "initialize-orchestration-db.py"}',
        '--config', str(config_file_path)
    ]
    subprocess.run(cmd, check=True)


if '__main__' == __name__:
    sys.exit(main(sys.argv))
