"""Dataset-manager verification helpers specific to the clp-json package."""

import logging
import re

import pytest

from tests.package_tests.classes import ClpPackage
from tests.package_tests.clp_json.utils.dataset_manager import (
    ClpPackageDatasetManagerCommand,
    DatasetManagerArgs,
)
from tests.utils.classes import ClpAction, ClpVerificationResult

logger = logging.getLogger(__name__)


def verify_dataset_manager_list(
    action: ClpAction,
    expected_datasets: list[str],
) -> ClpVerificationResult:
    """
    Verifies the dataset-manager list action by checking that the output dataset list matches the
    list of expected datasets.

    :param action:
    :param expected_datasets:
    :return: A `ClpVerificationResult` indicating the success or failure of the verification.
    """
    logger.info("Verifying dataset-manager 'list'.")

    returncode_result = action.verify_returncode()
    if not returncode_result:
        return returncode_result

    dataset_list = _extract_dataset_names_from_output(action)
    expected_sorted = sorted(expected_datasets)

    if dataset_list == expected_sorted:
        return action.pass_verification()

    return action.fail_verification(
        "Dataset-manager 'list' verification failure: mismatch between output dataset list"
        f" '{dataset_list}' and expected datasets '{expected_sorted}'"
    )


def verify_dataset_manager_del(
    action: ClpAction,
    clp_package: ClpPackage,
) -> ClpVerificationResult:
    """
    Verifies the dataset-manager del action by checking that the datasets specified for deletion are
    no longer present in the output of a supporting dataset-manager list command.

    :param action:
    :param clp_package:
    :return: A `ClpVerificationResult` indicating the success or failure of the verification.
    """
    logger.info("Verifying dataset-manager 'del'.")

    returncode_result = action.verify_returncode()
    if not returncode_result:
        return returncode_result

    args = action.args
    if not isinstance(args, DatasetManagerArgs):
        pytest.fail(
            "dataset-manager verification requires `ClpAction.args` to be a DatasetManagerArgs"
            " instance."
        )

    # Get list of all datasets currently in archives.
    list_args = DatasetManagerArgs(
        script_path=clp_package.path_config.dataset_manager_path,
        config=clp_package.temp_config_file_path,
        command=ClpPackageDatasetManagerCommand.LIST_COMMAND,
    )
    list_action = ClpAction.from_args(list_args)
    list_returncode_result = list_action.verify_returncode()
    if not list_returncode_result:
        return action.fail_verification(
            "During dataset-manager 'del' verification, supporting call to dataset-manager 'list'"
            " returned a non-zero exit code.",
            supporting_action=list_action,
        )

    current_datasets = _extract_dataset_names_from_output(list_action)

    datasets_specified_for_deletion = args.datasets or []
    if args.del_all:
        if len(current_datasets) > 0:
            return action.fail_verification(
                "Dataset-manager 'del --all' verification failure: There are datasets still"
                f" present in the database: '{current_datasets}'.",
                supporting_action=list_action,
            )
    elif any(item in current_datasets for item in datasets_specified_for_deletion):
        return action.fail_verification(
            "Dataset-manager 'del' verification failure: Some datasets that were specified for"
            " deletion are still present in the database.",
            supporting_action=list_action,
        )

    return action.pass_verification()


def _extract_dataset_names_from_output(
    action: ClpAction,
) -> list[str]:
    """Extracts the sorted list of dataset names from a dataset-manager 'list' action's output."""
    dataset_list: list[str] = []
    output = action.get_output()
    output_lines = output.splitlines()
    num_datasets: int | None = None
    for line in output_lines:
        match = re.search(r"Found (\d+) datasets", line)
        if match:
            num_datasets = int(match.group(1))
            output_lines.remove(line)
            break

    if num_datasets is None:
        pytest.fail(
            "Unable to parse dataset-manager list output: missing 'Found <N> datasets' marker."
        )

    if num_datasets == 0:
        return dataset_list

    for line in output_lines:
        match = re.search(r"INFO \[dataset_manager\] (.+)", line)
        if match:
            dataset_list.append(match.group(1))

    if len(dataset_list) != num_datasets:
        pytest.fail(
            "Unable to parse dataset-manager list output: reported dataset count does not match"
            " parsed entries."
        )

    return sorted(dataset_list)
