"""Functions and classes to facilitate clp-json dataset-manager."""

import logging
import re
from pathlib import Path

import pytest
from strenum import StrEnum

from tests.package_tests.classes import ClpPackage
from tests.utils.classes import (
    ClpAction,
    ClpVerificationResult,
    CmdArgs,
    SampleDataset,
)

logger = logging.getLogger(__name__)


class DatasetManagerArgs(CmdArgs):
    """Command argument model for dataset-manager."""

    script_path: Path
    config: Path
    subcommand: str
    del_all: bool = False
    datasets: list[str] | None = None

    def to_cmd(self) -> list[str]:
        """Converts the model attributes to a command list."""
        cmd = [str(self.script_path), "--config", str(self.config)]

        cmd.append(self.subcommand)

        if self.del_all:
            cmd.append("--all")
        if self.datasets:
            cmd.extend(self.datasets)

        return cmd


class ClpPackageDatasetManagerSubcommand(StrEnum):
    """Subcommands supported by dataset-manager."""

    LIST_COMMAND = "list"
    DEL_COMMAND = "del"


def dataset_manager_list(clp_package: ClpPackage) -> ClpAction:
    """
    Lists the datasets currently stored in package archives.

    :param clp_package:
    :return: The `ClpAction` instance that runs the list operation.
    """
    logger.info("Performing 'list' operation with dataset-manager.")

    path_config = clp_package.path_config
    args = DatasetManagerArgs(
        script_path=path_config.dataset_manager_path,
        config=clp_package.temp_config_file_path,
        subcommand=ClpPackageDatasetManagerSubcommand.LIST_COMMAND,
    )
    return ClpAction.from_args(args)


def dataset_manager_del(
    clp_package: ClpPackage,
    datasets_to_del: list[SampleDataset] | None = None,
    del_all: bool = False,
) -> ClpAction:
    """
    Deletes datasets from the package archives.

    :param clp_package:
    :param datasets_to_del:
    :param del_all:
    :return: The `ClpAction` instance that runs the delete operation.
    """
    logger.info("Performing 'del' operation with dataset-manager.")

    if datasets_to_del and del_all:
        pytest.fail(
            "You must specify either `datasets_to_del` or `del_all` for dataset-manager, not both."
        )
    if not datasets_to_del and not del_all:
        pytest.fail(
            "You must specify either `datasets_to_del` or `del_all` arguments for dataset-manager."
        )

    path_config = clp_package.path_config
    args = DatasetManagerArgs(
        script_path=path_config.dataset_manager_path,
        config=clp_package.temp_config_file_path,
        subcommand=ClpPackageDatasetManagerSubcommand.DEL_COMMAND,
        del_all=del_all,
    )
    if datasets_to_del:
        args.datasets = [dataset.dataset_name for dataset in datasets_to_del]

    return ClpAction.from_args(args)


def verify_dataset_manager_list_action_clp_json(
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


def verify_dataset_manager_del_action_clp_json(
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
    list_action = dataset_manager_list(clp_package=clp_package)
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
