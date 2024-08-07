#!/usr/bin/env python3
import argparse
import logging
import subprocess
import pathlib
import shutil
import sys
import uuid
import urllib.parse
import urllib.request

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
    args_parser.add_argument("git_root", help="root of repository")
    args_parser.add_argument("source_url", help="Directory to output the files.")
    args_parser.add_argument("source_name", help="Name of the source file.")
    args_parser.add_argument("dest_dir", help="Destination directory to output the files.")
    args_parser.add_argument("--extract", action='store_true', help="Whether to source needs to be extracted.")

    parsed_args = args_parser.parse_args(argv[1:])
    git_root = pathlib.Path(parsed_args.git_root)
    source_url = parsed_args.source_url
    source_name = parsed_args.source_name
    target_dest_path = pathlib.Path(parsed_args.dest_dir).resolve()
    extract_source = parsed_args.extract

    if git_root.exists() and git_root.is_dir():
        cmd = ["git", "submodule", "update", "--init", str(target_dest_path)]
        print(" ".join(cmd))
        subprocess.run(["git", "submodule", "update", "--init", str(target_dest_path)])
        return 0


    parsed_url = urllib.parse.urlparse(source_url)
    filename = pathlib.Path(parsed_url.path).name

    extraction_dir = pathlib.Path("/") / "tmp" / str(uuid.uuid4())
    extraction_dir.mkdir(parents=True, exist_ok=True)

    # Download file
    file_path = extraction_dir / filename
    urllib.request.urlretrieve(source_url, file_path)
    if extract_source:
        logger.debug(f"Extracting {file_path} to {extraction_dir}")
        # NOTE: We need to convert file_path to a str since unpack_archive only
        # accepts a path-like object on Python versions >= 3.7
        shutil.unpack_archive(str(file_path), extraction_dir)


    # Remove destination
    if target_dest_path.exists():
        print(f"removing {target_dest_path}")
        shutil.rmtree(target_dest_path, ignore_errors=True)
    else:
        # Create destination parent
        target_dest_parent = target_dest_path.parent
        print(f"mkdiring {target_dest_parent}")
        target_dest_parent.mkdir(parents=True, exist_ok=True)

    target_source_path = extraction_dir / source_name
    # Copy destination to target
    if extract_source:
        print(f"copying {target_source_path} to {target_dest_path}")
        shutil.copytree(target_source_path, target_dest_path)
    else:
        shutil.copy(target_source_path, target_dest_path)

    shutil.rmtree(extraction_dir)

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
