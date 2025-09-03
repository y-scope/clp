import argparse
import json
import logging
import sys
from pathlib import Path
from typing import Dict, List, TypedDict

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


class RangeMapping(TypedDict):
    lowerBound: str
    upperBound: str


class FilterRule(TypedDict):
    columnName: str
    rangeMapping: RangeMapping
    required: bool


MetadataFilterDict = Dict[str, List[FilterRule]]


def main(argv=None) -> int:
    if argv is None:
        argv = sys.argv

    args_parser = argparse.ArgumentParser(
        description="Generates a metadata filter file for all user-configured datasets."
    )
    args_parser.add_argument(
        "--clp-package-dir", help="CLP package directory.", required=True, type=Path
    )
    args_parser.add_argument(
        "--output-file", help="Path for the metadata filter file.", required=True, type=Path
    )

    parsed_args = args_parser.parse_args(argv[1:])
    clp_package_dir: Path = parsed_args.clp_package_dir.resolve()
    json_output_file: Path = parsed_args.output_file

    datasets: List[str] = []
    if not _get_dataset_names(clp_package_dir, datasets):
        return 1

    try:
        timestamp_keys_by_dataset = _prompt_timestamp_keys(datasets)
    except KeyboardInterrupt:
        logger.error("Interrupted while collecting timestamp keys.")
        return 1

    metadata_filters: MetadataFilterDict = {}
    if not _construct_metadata_filters(
        metadata_filters, datasets, timestamp_keys=timestamp_keys_by_dataset
    ):
        return 1

    with open(json_output_file, "w") as json_file:
        json.dump(metadata_filters, json_file, indent=2)

    return 0


def _get_dataset_names(clp_package_dir: Path, datasets: List[str]) -> bool:
    """
    Return the names of first-level subdirectories in <clp-package-dir>/var/data/archives. Each
    subdirectory name is treated as a dataset name.

    :param clp_package_dir:
    :param datasets:
    :return: True if dataset list is constructed successfully.
    :return: False on error.
    """
    archives_dir = clp_package_dir / "var" / "data" / "archives"

    if not archives_dir.exists():
        logger.error("Archives directory does not exist: %s", archives_dir)
        return False

    if not archives_dir.is_dir():
        logger.error("Archives path is not a directory: %s", archives_dir)
        return False

    dataset_names = sorted([p.name for p in archives_dir.iterdir() if p.is_dir()])
    if not dataset_names:
        logger.warning("No datasets found in %s. Did you compress any logs yet?", archives_dir)
        return False

    datasets[:] = dataset_names
    return True


def _prompt_timestamp_keys(
    datasets: List[str],
    *,
    default_key: str = "timestamp",
) -> Dict[str, str]:
    """
    Prompt the user to provide the timestamp key for each dataset. If the user presses Enter, falls
    back to `default_key`.

    :param datasets:
    :param default_key:
    :return: Dictionary mapping dataset names to their timestamp keys.
    """
    print("\nPlease enter the timestamp key that corresponds to each of your archived datasets.")
    print("Press <Enter> to accept the default key.\n")

    BOLD = "\033[1m"
    RESET = "\033[0m"

    data_time_pair: Dict[str, str] = {}
    for dataset in datasets:
        user_input = input(
            f">>> {BOLD}{dataset}{RESET} [default key: {BOLD}{default_key}{RESET}]: "
        )
        key = user_input if user_input else default_key
        data_time_pair[dataset] = key

    return data_time_pair


def _construct_metadata_filters(
    metadata_filters: MetadataFilterDict,
    datasets: List[str],
    *,
    timestamp_keys: Dict[str, str],
    rangeMap: RangeMapping = {
        "lowerBound": "begin_timestamp",
        "upperBound": "end_timestamp",
    },
    required: bool = False,
) -> bool:
    """
    Constructs a metadata filter for each dataset using a per-dataset timestamp key.

    :param metadata_filters:
    :param datasets:
    :param timestamp_keys: Mapping from dataset name -> timestamp key.
    :param lower_bound_key:
    :param upper_bound_key:
    :param required:
    :return: True on success, False on error.
    """

    if 0 == len(datasets):
        logger.error("No datasets provided to _construct_metadata_filters.")
        return False

    missing = [d for d in datasets if d not in timestamp_keys]
    if 0 < len(missing):
        logger.error("Missing timestamp key(s) for dataset(s): %s", ", ".join(missing))
        return False

    metadata_filters.clear()

    try:
        for dataset in datasets:
            range_mapping = rangeMap
            rule: FilterRule = {
                "columnName": timestamp_keys[dataset],
                "rangeMapping": range_mapping,
                "required": required,
            }
            metadata_filters[f"clp.default.{dataset}"] = [rule]
    except Exception:
        logger.exception("Unexpected error while constructing metadata filters.")
        metadata_filters.clear()
        return False

    return True


if "__main__" == __name__:
    sys.exit(main(sys.argv))
