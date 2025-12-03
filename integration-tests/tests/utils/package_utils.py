"""Provides utility functions for interacting with the CLP package."""

import json
import subprocess
from pathlib import Path
from typing import Final

import pytest

from tests.utils.asserting_utils import run_and_assert
from tests.utils.config import (
    IntegrationTestLogs,
    PackageCompressJob,
    PackageConfig,
    PackageInstance,
    PackageSearchJob,
    PrestoFilterJob,
)
from tests.utils.docker_utils import get_docker_binary_path
from tests.utils.utils import (
    resolve_path_env_var,
    validate_dir_exists,
    validate_file_exists,
)

DEFAULT_CMD_TIMEOUT_SECONDS = 120.0

# Datatypes and constants for constructing 'split-filter.json'.
SplitFilterRuleDict = dict[str, object]
SplitFilterDict = dict[str, SplitFilterRuleDict]

DEFAULT_REQUIRED: Final[bool] = False
DEFAULT_RANGE_MAPPING: Final[dict[str, str]] = {
    "lowerBound": "begin_timestamp",
    "upperBound": "end_timestamp",
}
DEFAULT_CUSTOM_OPTIONS: Final[dict[str, dict[str, str]]] = {"rangeMapping": DEFAULT_RANGE_MAPPING}


def start_clp_package(package_config: PackageConfig) -> None:
    """
    Starts an instance of the CLP package.

    :param package_config:
    :raise: Propagates `run_and_assert`'s errors.
    """
    path_config = package_config.path_config
    start_script_path = path_config.start_script_path
    temp_config_file_path = package_config.temp_config_file_path

    # fmt: off
    start_cmd = [
        str(start_script_path),
        "--config", str(temp_config_file_path),
    ]
    # fmt: on
    run_and_assert(start_cmd, timeout=DEFAULT_CMD_TIMEOUT_SECONDS)


def _construct_split_filters(dataset_timestamp_dict: dict[str, str]) -> SplitFilterDict:
    """
    Constructs a split filter for each dataset.

    :param dataset_timestamp_dict:
    :return: A `SplitFilterDict` object.
    """
    split_filters: SplitFilterDict = {}
    for dataset, timestamp_column_name in dataset_timestamp_dict.items():
        rule: SplitFilterRuleDict = {
            "columnName": timestamp_column_name,
            "customOptions": DEFAULT_CUSTOM_OPTIONS,
            "required": DEFAULT_REQUIRED,
        }
        split_filters[f"clp.default.{dataset}"] = rule

    return split_filters


def _write_split_filter_json(split_filters: SplitFilterDict, split_filter_file_path: Path) -> None:
    """
    Write split filter JSON with pretty formatting and a trailing newline.

    Empty dictionaries are written with opening and closing braces on separate lines.
    """
    with split_filter_file_path.open("w", encoding="utf-8") as split_filter_file:
        if not split_filters:
            split_filter_file.write("{\n}\n")
        else:
            json.dump(split_filters, split_filter_file, indent=2)
            split_filter_file.write("\n")


def start_presto_cluster(package_config: PackageConfig) -> None:
    """
    Starts a Presto cluster for the CLP package.

    :param package_config:
    """
    # Generate the necessary config for Presto to work with CLP.
    clp_repo_dir: Path = resolve_path_env_var("CLP_REPO_DIR")
    validate_dir_exists(clp_repo_dir)

    set_up_config_path = (
        clp_repo_dir / "tools" / "deployment" / "presto-clp" / "scripts" / "set-up-config.sh"
    )
    validate_file_exists(set_up_config_path)

    # fmt: off
    setup_presto_cmd = [
        str(set_up_config_path),
        str(package_config.path_config.clp_package_dir),
    ]
    # fmt: on
    subprocess.run(setup_presto_cmd, check=True)

    # Configure Presto to use CLP's metadata database by modifying 'split-filter.json'.
    dataset_timestamp_dict: dict[str, str] = {}
    package_job_list = package_config.package_job_list
    if package_job_list is not None:
        package_compress_jobs = package_job_list.package_compress_jobs
    else:
        package_compress_jobs = None

    if package_compress_jobs is not None:
        for package_compress_job in package_compress_jobs:
            dataset_name = package_compress_job.dataset_name
            timestamp_key = package_compress_job.timestamp_key

            # Skip jobs that do not specify both dataset_name and timestamp_key.
            if dataset_name is None or timestamp_key is None:
                continue

            dataset_timestamp_dict[dataset_name] = timestamp_key

        # Construct split filters.
        split_filters = _construct_split_filters(dataset_timestamp_dict)

        # Write 'split-filter.json'.
        split_filter_file_path = (
            clp_repo_dir
            / "tools"
            / "deployment"
            / "presto-clp"
            / "coordinator"
            / "config-template"
            / "split-filter.json"
        )
        validate_file_exists(split_filter_file_path)

        _write_split_filter_json(split_filters, split_filter_file_path)

    # Start up the Presto cluster.
    docker_bin = get_docker_binary_path()
    docker_compose_file_path = (
        clp_repo_dir / "tools" / "deployment" / "presto-clp" / "docker-compose.yaml"
    )
    validate_file_exists(docker_compose_file_path)

    start_presto_cmd = [
        docker_bin,
        "compose",
        "--file",
        str(docker_compose_file_path),
        "up",
        "--detach",
    ]
    subprocess.run(start_presto_cmd, check=True)


def stop_clp_package(package_config: PackageConfig) -> None:
    """
    Stops the running instance of the CLP package.

    :param package_config:
    :raise: Propagates `run_and_assert`'s errors.
    """
    path_config = package_config.path_config
    stop_script_path = path_config.stop_script_path
    temp_config_file_path = package_config.temp_config_file_path

    # fmt: off
    stop_cmd = [
        str(stop_script_path),
        "--config", str(temp_config_file_path),
    ]
    # fmt: on
    run_and_assert(stop_cmd, timeout=DEFAULT_CMD_TIMEOUT_SECONDS)


def stop_presto_cluster() -> None:
    """Stops a Presto cluster for the CLP package."""
    clp_repo_dir: Path = resolve_path_env_var("CLP_REPO_DIR")
    validate_dir_exists(clp_repo_dir)

    docker_bin = get_docker_binary_path()
    docker_compose_file_path = (
        clp_repo_dir / "tools" / "deployment" / "presto-clp" / "docker-compose.yaml"
    )
    validate_file_exists(docker_compose_file_path)

    stop_presto_cmd = [docker_bin, "compose", "--file", str(docker_compose_file_path), "down"]
    subprocess.run(stop_presto_cmd, check=True)

    # Restore split-filter.json file to what it was pre-run.
    split_filter_file_path = (
        clp_repo_dir
        / "tools"
        / "deployment"
        / "presto-clp"
        / "coordinator"
        / "config-template"
        / "split-filter.json"
    )
    validate_file_exists(split_filter_file_path)

    empty_json: SplitFilterDict = {}
    _write_split_filter_json(empty_json, split_filter_file_path)


def compress_with_clp_package(
    request: pytest.FixtureRequest,
    compress_job: PackageCompressJob,
    package_instance: PackageInstance,
) -> None:
    """
    Construct and run a compression command for the CLP package.

    :param request:
    :param compress_job:
    :param package_instance:
    """
    package_config = package_instance.package_config
    compress_script_path = package_config.path_config.compress_script_path
    temp_config_file_path = package_config.temp_config_file_path
    # Get the correct logs fixture for this job and set up path config objects.
    integration_test_logs: IntegrationTestLogs = request.getfixturevalue(
        compress_job.log_fixture_name
    )

    # Construct the compression command for this job.
    compress_cmd = [
        str(compress_script_path),
        "--config",
        str(temp_config_file_path),
    ]

    if compress_job.dataset_name is not None:
        compress_cmd.extend(
            [
                "--dataset",
                compress_job.dataset_name,
            ]
        )

    if compress_job.timestamp_key is not None:
        compress_cmd.extend(
            [
                "--timestamp-key",
                compress_job.timestamp_key,
            ]
        )

    if compress_job.unstructured:
        compress_cmd.append("--unstructured")

    if compress_job.tags is not None:
        compress_cmd.extend(
            [
                "-t",
                ",".join(compress_job.tags),
            ]
        )

    if compress_job.subpath is not None:
        compress_cmd.append(str(integration_test_logs.extraction_dir / compress_job.subpath))
    else:
        compress_cmd.append(str(integration_test_logs.extraction_dir))

    # Run compression command for this job and assert that it succeeds.
    run_and_assert(compress_cmd)


def search_with_clp_package(
    request: pytest.FixtureRequest,
    search_job: PackageSearchJob,
    package_instance: PackageInstance,
) -> None:
    """
    Construct and run a search command for the CLP package.

    :param request:
    :param search_job:
    :param package_instance:
    """
    package_config = package_instance.package_config
    search_script_path = package_config.path_config.search_script_path
    temp_config_file_path = package_config.temp_config_file_path

    # Construct the search command for this job.
    search_cmd = [
        str(search_script_path),
        "--config",
        str(temp_config_file_path),
    ]
    if search_job.package_compress_job.dataset_name is not None:
        search_cmd.extend(
            [
                "--dataset",
                search_job.package_compress_job.dataset_name,
            ]
        )
    if search_job.package_compress_job.tags is not None:
        search_cmd.extend(
            [
                "-t",
                ",".join(search_job.package_compress_job.tags),
            ]
        )
    if search_job.begin_time is not None:
        search_cmd.extend(
            [
                "--begin-time",
                str(search_job.begin_time),
            ]
        )
    if search_job.end_time is not None:
        search_cmd.extend(
            [
                "--end-time",
                str(search_job.end_time),
            ]
        )
    if search_job.ignore_case:
        search_cmd.append("--ignore-case")
    if search_job.file_subpath is not None:
        # Resolve the original logs root using the same fixture as the compress job.
        integration_test_logs: IntegrationTestLogs = request.getfixturevalue(
            search_job.package_compress_job.log_fixture_name
        )

        # file_subpath is interpreted as relative to the extraction_dir of this dataset.
        file_subpath = integration_test_logs.extraction_dir / search_job.file_subpath

        search_cmd.extend(
            [
                "--file-path",
                str(file_subpath),
            ]
        )
    if search_job.count:
        search_cmd.append("--count")
    if search_job.count_by_time is not None:
        search_cmd.extend(
            [
                "--count-by-time",
                str(search_job.count_by_time),
            ]
        )
    search_cmd.append("--raw")
    search_cmd.append(search_job.wildcard_query)

    # Run search command for this job and capture the output.
    completed_process = run_and_assert(
        search_cmd,
        capture_output=True,
        text=True,
    )

    # Compare captured output to `desired_result`.
    search_output = completed_process.stdout
    expected_output = search_job.desired_result

    if search_output != expected_output:
        error_message = (
            f"Search output for job '{search_job.job_name}' did not match desired_result.\n"
            "Expected:\n"
            f"{expected_output}\n"
            "Actual:\n"
            f"{search_output}"
        )
        pytest.fail(error_message)


def run_presto_filter(
    request: pytest.FixtureRequest,
    presto_filter: PrestoFilterJob,
    package_instance: PackageInstance,
) -> None:
    """
    Run a Presto filter.

    :param request:
    :param presto_filter:
    :param package_instance:
    """
    # Start up the Presto CLI.

    # Run the filter.

    # Get the result and verify its correctness.

    # Shut down the Presto CLI.
    
    return
