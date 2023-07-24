#!/usr/bin/env python3
import argparse
import hashlib
import json
import logging
import mmap
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


def hash_file(algo: str, path: pathlib.Path):
    if "sha3_256" == algo:
        hasher = hashlib.sha3_256()
        with open(path, 'rb') as f:
            with mmap.mmap(f.fileno(), 0, prot=mmap.PROT_READ) as mapped_file:
                hasher.update(mapped_file)
        return hasher.hexdigest()


def main(argv):
    script_dir = pathlib.Path(__file__).parent.resolve()
    project_root_dir = script_dir.parent.parent.parent

    args_parser = argparse.ArgumentParser(description="Download dependency.")
    args_parser.add_argument("config-file", help="Dependency configuration file.")

    parsed_args = vars(args_parser.parse_args(argv[1:]))
    config_file_path = pathlib.Path(parsed_args["config-file"]).resolve()

    # Load configurations
    with open(config_file_path) as f:
        config = json.load(f)

    target_url = config["url"]
    parsed_url = urllib.parse.urlparse(target_url)
    filename = pathlib.Path(parsed_url.path).name

    extraction_dir = pathlib.Path("/") / "tmp" / str(uuid.uuid4())
    extraction_dir.mkdir(parents=True, exist_ok=True)

    # Download file
    file_path = extraction_dir / filename
    urllib.request.urlretrieve(target_url, file_path)
    if config["unzip"]:
        # NOTE: We need to convert file_path to a str since unpack_archive only
        # accepts a path-like object on Python versions >= 3.7
        shutil.unpack_archive(str(file_path), extraction_dir)

    if "hash" in config:
        # Verify hash
        hash = hash_file(config["hash"]["algo"], file_path)
        if hash != config["hash"]["digest"]:
            logger.fatal("Hash mismatch.")
            return -1

    for target in config["targets"]:
        target_source_path = extraction_dir / target["source"]
        target_dest_path = project_root_dir / target["destination"]

        target_dest_parent = target_dest_path.parent

        # Remove destination
        if target_dest_path.exists():
            shutil.rmtree(target_dest_path, ignore_errors=True)
        else:
            # Create destination parent
            target_dest_parent.mkdir(parents=True, exist_ok=True)

        # Copy destination to target
        if config["unzip"]:
            shutil.copytree(target_source_path, target_dest_path)
        else:
            shutil.copy(target_source_path, target_dest_path)

    shutil.rmtree(extraction_dir)

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
