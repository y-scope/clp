"""Functions and classes to facilitate clp-json dataset-manager."""

import logging
import re
from pathlib import Path

import pytest
from strenum import StrEnum

from tests.package_tests.classes import ClpPackage
from tests.utils.classes import CmdArgs, ExternalAction, IntegrationTestDataset, VerificationResult
from tests.utils.logging_utils import format_action_failure_msg

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


def dataset_manager_list(clp_package: ClpPackage) -> ExternalAction:
    """
    Lists the datasets currently stored in package archives.

    :param clp_package:
    :return: The `ExternalAction` instance that runs the list operation.
    """
    logger.info("Performing 'list' operation with dataset-manager.")

    path_config = clp_package.path_config
    args = DatasetManagerArgs(
        script_path=path_config.dataset_manager_path,
        config=clp_package.temp_config_file_path,
        subcommand=ClpPackageDatasetManagerSubcommand.LIST_COMMAND,
    )
    return ExternalAction(cmd=args.to_cmd(), args=args)


def dataset_manager_del(
    clp_package: ClpPackage,
    datasets_to_del: list[IntegrationTestDataset] | None = None,
    del_all: bool = False,
) -> ExternalAction:
    """
    Deletes datasets from the package archives.

    :param clp_package:
    :param datasets_to_del:
    :param del_all:
    :return: The `ExternalAction` instance that runs the delete operation.
    """
    logger.info("Performing 'del' operation with dataset-manager.")

    path_config = clp_package.path_config
    args = DatasetManagerArgs(
        script_path=path_config.dataset_manager_path,
        config=clp_package.temp_config_file_path,
        subcommand=ClpPackageDatasetManagerSubcommand.DEL_COMMAND,
        del_all=del_all,
    )
    if datasets_to_del:
        args.datasets = [dataset.dataset_name for dataset in datasets_to_del]
    elif not del_all:
        pytest.fail(
            "You must specify either `datasets_to_del` or `del_all` arguments for dataset-manager."
        )

    return ExternalAction(cmd=args.to_cmd(), args=args)


def verify_dataset_manager_list_action_clp_json(
    action: ExternalAction,
    expected_datasets: list[str],
) -> VerificationResult:
    """
    Verifies the dataset-manager list action by checking that the output dataset list matches the
    list of expected datasets.

    :param action:
    :param expected_datasets:
    :return: A `VerificationResult` indicating the success or failure of the verification.
    """
    logger.info("Verifying dataset-manager 'list'.")

    if action.completed_proc.returncode != 0:
        return VerificationResult.fail(
            format_action_failure_msg(
                "The 'dataset-manager.sh list' subprocess returned a non-zero exit code.",
                action,
            )
        )

    dataset_list = _extract_dataset_names_from_output(action)
    expected_sorted = sorted(expected_datasets)

    if dataset_list == expected_sorted:
        return VerificationResult.ok()

    return VerificationResult.fail(
        format_action_failure_msg(
            "Dataset-manager 'list' verification failure: mismatch between output dataset list"
            f" '{dataset_list}' and expected datasets '{expected_sorted}'",
            action,
        )
    )


def verify_dataset_manager_del_action_clp_json(
    action: ExternalAction,
    clp_package: ClpPackage,
) -> VerificationResult:
    """
    Verifies the dataset-manager del action by checking that the datasets specified for deletion are
    no longer present in the output of a supporting dataset-manager list command.

    :param action:
    :param clp_package:
    :return: A `VerificationResult` indicating the success or failure of the verification.
    """
    logger.info("Verifying dataset-manager 'del'.")

    if action.completed_proc.returncode != 0:
        return VerificationResult.fail(
            format_action_failure_msg(
                "The 'dataset-manager.sh del' subprocess returned a non-zero exit code.",
                action,
            )
        )

    # Get list of all datasets currently in archives.
    list_action = dataset_manager_list(clp_package=clp_package)
    if list_action.completed_proc.returncode != 0:
        pytest.fail(
            "During dataset-manager 'del' verification, supporting call to dataset-manager 'list'"
            f" returned a non-zero exit code. Subprocess log: '{list_action.log_file_path}'"
        )

    current_datasets = _extract_dataset_names_from_output(list_action)

    args = action.args
    assert isinstance(args, DatasetManagerArgs)
    datasets_specified_for_deletion = args.datasets or []
    del_all_flag = args.del_all
    if del_all_flag:
        if len(current_datasets) > 0:
            return VerificationResult.fail(
                format_action_failure_msg(
                    f"Dataset-manager 'del --all' verification failure: There are datasets still"
                    f" present in the database: '{current_datasets}'.",
                    action,
                )
            )
    elif any(item in current_datasets for item in datasets_specified_for_deletion):
        return VerificationResult.fail(
            format_action_failure_msg(
                "Dataset-manager 'del' verification failure: Some datasets that were specified for"
                " deletion are still present in the database.",
                action,
            )
        )

    return VerificationResult.ok()


def _extract_dataset_names_from_output(
    action: ExternalAction,
) -> list[str]:
    """Extracts the sorted list of dataset names from a dataset-manager 'list' action's output."""
    dataset_list: list[str] = []
    output = action.get_output()
    output_lines = output.splitlines()
    num_datasets = 0
    for line in output_lines:
        match = re.search(r"Found (\d+) datasets", line)
        if match:
            num_datasets = int(match.group(1))
            output_lines.remove(line)
            break

    if num_datasets == 0:
        return dataset_list

    for line in output_lines:
        match = re.search(r"INFO \[dataset_manager\] (.+)", line)
        if match:
            dataset_list.append(match.group(1))

    return sorted(dataset_list)
