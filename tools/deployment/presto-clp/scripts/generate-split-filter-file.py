import argparse
import json
import logging
import sys
from pathlib import Path
from typing import Dict, Final, List, Optional, TypedDict

# CONSTANTS
ANSI_BOLD: Final[str] = "\033[1m"
ANSI_RESET: Final[str] = "\033[0m"
DEFAULT_TIMESTAMP_KEY: Final[str] = "timestamp"
DEFAULT_REQUIRED: Final[bool] = False

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


class CustomOptions(TypedDict):
    rangeMapping: RangeMapping


class SplitFilterRule(TypedDict):
    columnName: str
    customOptions: CustomOptions
    required: bool


SplitFilterDict = Dict[str, List[SplitFilterRule]]
DEFAULT_RANGE_MAPPING: Final[RangeMapping] = {
    "lowerBound": "begin_timestamp",
    "upperBound": "end_timestamp",
}
DEFAULT_CUSTOM_OPTIONS: Final[CustomOptions] = {"rangeMapping": DEFAULT_RANGE_MAPPING}


def validate_dir(path: Path, label: str) -> bool:
    """
    Determines whether or not the path exists and whether or not it is a directory. If either of
    those are false, logs error message with path and returns False.

    :param path:
    :param label:
    :param logger:
    :return: True if path exists and is a directory; False if either of those are not true.
    """
    if not path.exists():
        logger.error("%s directory does not exist: %s", label, path)
        return False
    if not path.is_dir():
        logger.error("%s path is not a directory: %s", label, path)
        return False
    return True


def main(argv=None) -> int:
    if argv is None:
        argv = sys.argv

    args_parser = argparse.ArgumentParser(
        description="Generates a split filter file for all user-configured datasets."
    )
    args_parser.add_argument(
        "--clp-package-dir", help="CLP package directory.", required=True, type=Path
    )
    args_parser.add_argument(
        "--output-file", help="Path for the split filter file.", required=True, type=Path
    )

    parsed_args = args_parser.parse_args(argv[1:])
    clp_package_dir: Path = parsed_args.clp_package_dir.resolve()
    archives_dir = clp_package_dir / "var" / "data" / "archives"
    json_output_file: Path = parsed_args.output_file.resolve()
    out_dir = json_output_file.parent

    if not validate_dir(archives_dir, "Archives"):
        return 1
    if not validate_dir(out_dir, "Output"):
        return 1

    if json_output_file.exists() and json_output_file.is_dir():
        logger.error("Output path is a directory: %s", json_output_file)
        return 1

    datasets = _get_dataset_names(archives_dir)
    if datasets == None:
        logger.error("No datasets found in %s. Did you compress any logs yet?", archives_dir)
        return 1

    try:
        timestamp_keys_by_dataset = _prompt_timestamp_keys(datasets)
    except KeyboardInterrupt:
        logger.error("Interrupted while collecting timestamp keys.")
        return 1

    split_filters = _construct_split_filters(datasets, timestamp_keys_by_dataset)
    if split_filters is None:
        logger.error("Missing timestamp key(s) for dataset(s).")
        return 1

    try:
        with open(json_output_file, "w") as json_file:
            json.dump(split_filters, json_file, indent=2)
    except OSError as e:
        logger.error("Failed to write output file %s: %s", json_output_file, e)
        return 1

    return 0


def _get_dataset_names(archives_dir: Path) -> Optional[List[str]]:
    """
    Return the names of first-level subdirectories in <clp-package-dir>/var/data/archives. Each
    subdirectory name is treated as a dataset name.

    :param archives_dir:
    :return: List of dataset names. None if there are no directories within
    <clp-package-dir>/var/data/archives.
    """

    datasets = sorted([p.name for p in archives_dir.iterdir() if p.is_dir()])
    return datasets if len(datasets) >= 1 else None


def _prompt_timestamp_keys(datasets: List[str]) -> Dict[str, str]:
    """
    Prompt the user to provide the timestamp key for each dataset. If the user doesn't provide one,
    falls back to `DEFAULT_TIMESTAMP_KEY`.

    :param datasets:
    :return: mapping of `dataset` -> `timestamp_keys`.
    """
    print(
        "\nPlease enter the timestamp key that corresponds to each of your archived datasets."
        "\nPress <Enter> to accept the default key.\n"
    )

    data_time_pairs: Dict[str, str] = {}
    for dataset in datasets:
        user_input = input(
            f">>> {ANSI_BOLD}{dataset}{ANSI_RESET} [default key: {ANSI_BOLD}{DEFAULT_TIMESTAMP_KEY}{ANSI_RESET}]: "
        )
        key = DEFAULT_TIMESTAMP_KEY if 0 == len(user_input) else user_input
        data_time_pairs[dataset] = key

    return data_time_pairs


def _construct_split_filters(
    datasets: List[str],
    timestamp_keys: Dict[str, str],
) -> Optional[SplitFilterDict]:
    """
    Constructs a split filter for each dataset using a per-dataset timestamp key.

    :param datasets:
    :param timestamp_keys: Mapping from dataset name -> timestamp key.
    :return: A SplitFilterDict containing all the SplitFilterRule objects for the JSON file.
    :return: A `SplitFilterDict` containing all the `SplitFilterRule` objects for the JSON file, or
    None if there are any datasets that don't have an associated timestamp key.
    """

    missing = [d for d in datasets if d not in timestamp_keys]
    if len(missing) != 0:
        logger.error("Missing timestamp key(s) for dataset(s): %s", ", ".join(missing))
        return None

    split_filters: SplitFilterDict = {}
    for dataset in datasets:
        rule: SplitFilterRule = {
            "columnName": timestamp_keys[dataset],
            "customOptions": DEFAULT_CUSTOM_OPTIONS,
            "required": DEFAULT_REQUIRED,
        }
        split_filters[f"clp.default.{dataset}"] = [rule]

    return split_filters


if "__main__" == __name__:
    sys.exit(main(sys.argv))
