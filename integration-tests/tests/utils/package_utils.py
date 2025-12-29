"""Provides utility functions for interacting with the CLP package."""

import json
import subprocess
import tempfile
from pathlib import Path
from typing import Any

import pytest
from clp_py_utils.clp_config import (
    CLP_DEFAULT_ARCHIVES_DIRECTORY_PATH,
    PRESTO_COORDINATOR_COMPONENT_NAME,
)

from tests.utils.asserting_utils import run_and_assert
from tests.utils.config import (
    PackageCompressionJob,
    PackageConfig,
    PackageInstance,
    PackagePostCompressionJob,
    PrestoFilterJob,
)
from tests.utils.utils import (
    get_binary_path,
    is_dir_tree_content_equal,
    resolve_path_env_var,
    unlink,
    validate_dir_exists,
    validate_file_exists,
)

DEFAULT_CMD_TIMEOUT_SECONDS = 120.0


def run_package_compression_script(
    compression_job: PackageCompressionJob,
    package_instance: PackageInstance,
) -> None:
    """
    Construct and run a compression command for the CLP package.

    :param compression_job:
    :param package_instance:
    """
    package_config = package_instance.package_config
    clp_package_dir = package_config.path_config.clp_package_dir
    compression_script_path = clp_package_dir / compression_job.script_path
    temp_config_file_path = package_config.temp_config_file_path

    # Construct the compression command for this job.
    compression_cmd = [
        str(compression_script_path),
        "--config",
        str(temp_config_file_path),
    ]

    if compression_job.flags is not None:
        for flag_name, flag_value in compression_job.flags.items():
            compression_cmd.append(flag_name)
            if flag_value is not None:
                compression_cmd.append(str(flag_value))

    if compression_job.args is not None:
        compression_cmd.extend(compression_job.args)

    log_path = resolve_path_env_var("TEST_LOGS_DIR") / compression_job.log_path
    compression_cmd.append(str(log_path))

    # Run compression command for this job and assert that it succeeds.
    run_and_assert(compression_cmd)

    # Assert that the compression job was successful with package decompression.
    if compression_job.mode in ("clp-json", "clp-presto"):
        # TODO: Waiting for PR 1299 to be merged.
        assert True
    elif compression_job.mode == "clp-text":
        # Construct decompression command.
        decompress_script_path = package_config.path_config.decompress_script_path
        decompression_dir = package_config.path_config.package_decompression_dir
        decompress_cmd = [
            str(decompress_script_path),
            "--config",
            str(temp_config_file_path),
            "x",
            "--extraction-dir",
            str(decompression_dir),
        ]

        # Run decompression command and assert that it succeeds.
        run_and_assert(decompress_cmd)

        # output_path needs log_path attached to the end because the output of clp-text
        # decompression has a directory structure that is equivalent to log_path, but stored at
        # decompression_dir. e.g. if log_path was /a/b/c, and decompression_dir was /d/e/f, the
        # decompressed files would be stored at /d/e/f/a/b/c.
        output_path = decompression_dir / log_path

        assert is_dir_tree_content_equal(
            log_path,
            output_path,
        ), f"Mismatch between clp input {log_path} and output {output_path}."

        unlink(decompression_dir)


def run_package_script(
    post_compression_job: PackagePostCompressionJob,
    package_instance: PackageInstance,
) -> None:
    """
    Construct and run a command for the CLP package.

    :param post_compression_job:
    :param package_instance:
    """
    clp_package_dir = package_instance.package_config.path_config.clp_package_dir
    script_path = clp_package_dir / post_compression_job.script_path
    temp_config_file_path = package_instance.package_config.temp_config_file_path

    # Construct the command for this job.
    cmd = [
        str(script_path),
        "--config",
        str(temp_config_file_path),
    ]

    if post_compression_job.flags is not None:
        for flag_name, flag_value in post_compression_job.flags.items():
            cmd.append(flag_name)
            if flag_value is not None:
                cmd.append(str(flag_value))

    if post_compression_job.args is not None:
        cmd.extend(post_compression_job.args)

    if post_compression_job.requires_archive_id:
        archive_id = _get_archive_id(post_compression_job)
        cmd.append(archive_id)

    # Run command for this job and assert that it does not encounter errors.
    job_process = run_and_assert(cmd, capture_output=True, text=True)

    # Compare captured output with ground truth.
    ground_truth_file_path = (
        resolve_path_env_var("TEST_GROUND_TRUTHS_DIR")
        / post_compression_job.output_ground_truth_file
    )
    if not _compare_output_with_ground_truth(
        output=job_process.stdout, ground_truth_file_path=ground_truth_file_path
    ):
        pytest.fail(
            f"The output for job '{post_compression_job.job_name}' does not match the ground truth"
            f" stored in '{ground_truth_file_path}'"
        )


def run_presto_filter(
    presto_filter_job: PrestoFilterJob,
) -> None:
    """
    Run a Presto filter.

    :param request:
    :param presto_filter:
    :param package_instance:
    """
    # Run the filter in the Presto CLI.
    clp_repo_dir: Path = resolve_path_env_var("CLP_REPO_DIR")
    validate_dir_exists(clp_repo_dir)

    docker_bin = get_binary_path("docker")
    docker_compose_file_path = (
        clp_repo_dir / "tools" / "deployment" / "presto-clp" / "docker-compose.yaml"
    )
    validate_file_exists(docker_compose_file_path)

    presto_filter = presto_filter_job.filter

    presto_filter_cmd = [
        docker_bin,
        "compose",
        "--file",
        str(docker_compose_file_path),
        "exec",
        "-T",
        PRESTO_COORDINATOR_COMPONENT_NAME,
        "presto-cli",
        "--catalog",
        "clp",
        "--schema",
        "default",
        "--output-format",
        "ALIGNED",
        "--execute",
        presto_filter,
    ]

    # Run the filter and capture its output.
    presto_filter_process = run_and_assert(presto_filter_cmd, capture_output=True, text=True)

    # Check that the output matches ground truth.
    ground_truth_file_path = presto_filter_job.output_ground_truth_file
    if not _compare_output_with_ground_truth(
        output=presto_filter_process.stdout, ground_truth_file_path=ground_truth_file_path
    ):
        pytest.fail(
            f"The output for job '{presto_filter_job.job_name}' does not match the ground truth"
            f" stored in '{ground_truth_file_path}'"
        )


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
        package_compression_jobs = package_job_list.package_compression_jobs
    else:
        package_compression_jobs = None

    if package_compression_jobs is not None:
        for package_compression_job in package_compression_jobs:
            flags = package_compression_job.flags
            if flags is not None:
                dataset_name = flags["--dataset"]
                timestamp_key = flags["--timestamp-key"]

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
    docker_bin = get_binary_path("docker")
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

    docker_bin = get_binary_path("docker")
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

    empty_json: dict[str, Any] = {}
    _write_split_filter_json(empty_json, split_filter_file_path)


def _compare_output_with_ground_truth(output: str, ground_truth_file_path: Path) -> bool:
    with tempfile.TemporaryDirectory() as temporary_output_directory_name:
        # Write output to tempfile.
        output_file_path = Path(temporary_output_directory_name) / "output.tmp"
        output_file_path.write_text(output)

        # Compare tempfile with ground truth.
        return is_dir_tree_content_equal(output_file_path, ground_truth_file_path)


def _construct_split_filters(dataset_timestamp_dict: dict[str, str]) -> dict[str, Any]:
    """
    Constructs a split filter for each dataset.

    :param dataset_timestamp_dict:
    :return: A dictionary mapping dataset identifiers to split filters.
    """
    split_filters: dict[str, Any] = {}

    for dataset, timestamp_key in dataset_timestamp_dict.items():
        split_filters[f"clp.default.{dataset}"] = [
            {
                "columnName": timestamp_key,
                "customOptions": {
                    "rangeMapping": {"lowerBound": "begin_timestamp", "upperBound": "end_timestamp"}
                },
                "required": "false",
            }
        ]

    return split_filters


def _get_archive_id(post_compression_job: PackagePostCompressionJob) -> str:
    """
    Finds an archive ID from the default archives directory in the CLP package. Chooses the first
    one it finds.

    :param post_compression_job:
    :return: An archive ID.
    """
    dataset_name: str | None = None

    job_flags = post_compression_job.flags
    if job_flags is not None and "--dataset" in job_flags:
        dataset_name = str(job_flags["--dataset"])

    directory = resolve_path_env_var("CLP_PACKAGE_DIR") / CLP_DEFAULT_ARCHIVES_DIRECTORY_PATH
    if dataset_name is not None:
        directory /= dataset_name

    if not directory.is_dir():
        err_msg = f"Archive directory not found: {directory}"
        raise FileNotFoundError(err_msg)

    for child in directory.iterdir():
        if child.is_dir():
            return child.name

    err_msg = f"Archive directory '{directory}' contains no archive subdirectories."
    raise ValueError(err_msg)


def _write_split_filter_json(split_filters: dict[str, Any], split_filter_file_path: Path) -> None:
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
