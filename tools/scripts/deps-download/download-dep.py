#!/usr/bin/env python3
import argparse
import logging
import os
import shutil
import subprocess
import sys
import tempfile
import urllib.parse
import urllib.request
from pathlib import Path

# Setup logging
# Create logger
logger = logging.getLogger()
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def main(argv):
    args_parser = argparse.ArgumentParser(description="Download dependency.")
    args_parser.add_argument("source_url", help="URL of the source file.")
    args_parser.add_argument("source_name", help="Name of the source file.")
    args_parser.add_argument("dest_dir", help="Output directory for the download.")
    args_parser.add_argument("--extract", action="store_true", help="Extract the source file.")
    args_parser.add_argument(
        "--no-submodule",
        action="store_false",
        dest="use_submodule",
        help="Don't use git submodule update",
    )

    parsed_args = args_parser.parse_args(argv[1:])
    source_url = parsed_args.source_url
    source_name = parsed_args.source_name
    dest_dir = Path(parsed_args.dest_dir).resolve()
    extract_source = parsed_args.extract

    script_path = Path(os.path.realpath(__file__))
    git_dir = script_path.parent / ".." / ".." / ".." / ".git"
    if git_dir.exists() and git_dir.is_dir():
        if parsed_args.use_submodule:
            cmd = ["git", "submodule", "update", "--init", "--recursive", str(dest_dir)]
            try:
                subprocess.run(cmd, check=True)
            except subprocess.CalledProcessError:
                logger.exception(f"Failed to update submodule '{dest_dir}'.")
                return -1
            return 0

    with tempfile.TemporaryDirectory() as work_dir_name:
        work_dir = Path(work_dir_name)

        parsed_url = urllib.parse.urlparse(source_url)
        filename = Path(parsed_url.path).name
        file_path = work_dir / filename

        # Download file
        urllib.request.urlretrieve(source_url, file_path)
        if extract_source:
            # NOTE: We need to convert `file_path` to a str since `unpack_archive` only accepts a
            # path-like object on Python versions >= 3.7
            shutil.unpack_archive(str(file_path), work_dir)

        if dest_dir.exists():
            shutil.rmtree(dest_dir, ignore_errors=True)
        else:
            dest_dir.parent.mkdir(parents=True, exist_ok=True)

        source_path = work_dir / source_name
        if not source_path.exists():
            logger.error(f"Source '{source_path}' doesn't exist.")
            return -1

        if extract_source:
            shutil.copytree(source_path, dest_dir)
        else:
            shutil.copy(source_path, dest_dir)

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
