"""Tests for the clp-json package."""

import json
import logging
import uuid
from pathlib import Path
from typing import Any

import pytest
from pymongo import MongoClient

from tests.package_tests.classes import ClpPackage
from tests.package_tests.clp_json.utils.mode import CLP_JSON_MODE
from tests.package_tests.clp_json.verification.compress import (
    verify_compress_structured_clp_json,
    verify_compress_unstructured_clp_json,
)
from tests.package_tests.utils.compress import (
    CompressArgs,
)
from tests.utils.classes import (
    ClpAction,
    SampleDataset,
    SampleDatasetMetadata,
)
from tests.utils.config import ClpCorePathConfig

logger = logging.getLogger(__name__)


# Pytest markers for this module.
pytestmark = [
    pytest.mark.package,
    pytest.mark.clp_json,
    pytest.mark.parametrize(
        "clp_package", [CLP_JSON_MODE], indirect=True, ids=[CLP_JSON_MODE.mode_name]
    ),
]


@pytest.mark.startstop
def test_clp_json_startstop(clp_package: ClpPackage) -> None:
    """
    Validate that the `clp-json` package starts up successfully.

    :param clp_package:
    """
    assert clp_package


@pytest.mark.compression
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_compression_json_multifile(
    clp_package: ClpPackage,
    json_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package successfully compresses the `json-multifile` dataset.

    :param clp_package:
    :param json_multifile:
    """
    metadata: SampleDatasetMetadata = json_multifile.metadata
    args: CompressArgs = CompressArgs(
        script_path=clp_package.path_config.compress_path,
        config=clp_package.temp_config_file_path,
        dataset=metadata.dataset_name,
        timestamp_key=metadata.timestamp_key,
        unstructured=metadata.unstructured,
        paths=[json_multifile.logs_path],
    )

    logger.info("Compressing the 'json_multifile' dataset with the 'clp-json' package.")
    action = ClpAction.from_args(args)
    result = action.verify_returncode()
    assert result, result.failure_message

    logger.info("Verifying the compression of the 'json_multifile' dataset.")
    result = verify_compress_structured_clp_json(action, clp_package, json_multifile)
    assert result, result.failure_message


@pytest.mark.compression
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_compression_text_multifile(
    clp_package: ClpPackage,
    text_multifile: SampleDataset,
) -> None:
    """
    Validate that the `clp-json` package successfully compresses the `text-multifile` dataset as
    unstructured text.

    :param clp_package:
    :param text_multifile:
    """
    metadata: SampleDatasetMetadata = text_multifile.metadata
    args: CompressArgs = CompressArgs(
        script_path=clp_package.path_config.compress_path,
        config=clp_package.temp_config_file_path,
        dataset=metadata.dataset_name,
        timestamp_key=metadata.timestamp_key,
        unstructured=metadata.unstructured,
        paths=[text_multifile.logs_path],
    )

    logger.info("Compressing the 'text_multifile' dataset with the 'clp-json' package.")
    action = ClpAction.from_args(args)
    result = action.verify_returncode()
    assert result, result.failure_message

    logger.info("Verifying the compression of the 'text_multifile' dataset.")
    result = verify_compress_unstructured_clp_json(action, clp_package, text_multifile)
    assert result, result.failure_message


@pytest.mark.search
@pytest.mark.usefixtures("clear_package_archives")
def test_clp_json_search(clp_package: ClpPackage) -> None:
    """
    Validate that the `clp-json` package successfully searches some dataset.

    :param clp_package:
    """
    # TODO: compress a dataset

    # TODO: check the correctness of the compression

    # TODO: search through that dataset and check the correctness of the search results.

    assert clp_package


@pytest.mark.search
@pytest.mark.parametrize("limit", [None, 7, 2_000])
def test_results_cache_optional_limit(
    clp_package: ClpPackage,
    clp_core_path_config: ClpCorePathConfig,
    tmp_path: Path,
    limit: int | None,
) -> None:
    """An omitted limit returns over 1,000 events; a positive limit retains the latest events."""
    num_events = 1_205
    timestamps = [1_700_000_000_000 + index * 1_000 for index in range(num_events)]
    source = tmp_path / "input.jsonl"
    archives = tmp_path / "archives"
    # Alternate schemas to exercise a limit spanning multiple ERTs.
    source.write_text(
        "".join(
            json.dumps({"timestamp": timestamp, "message": "match", f"field{index % 2}": index})
            + "\n"
            for index, timestamp in enumerate(timestamps)
        )
    )
    binary = clp_core_path_config.clp_s_binary_path
    compression = [str(binary), "c", "--timestamp-key", "timestamp"]
    result = ClpAction.from_cmd([*compression, str(archives), str(source)]).verify_returncode()
    assert result, result.failure_message
    archive_paths = [path for path in archives.iterdir() if path.is_dir()]
    assert len(archive_paths) == 1

    cache = clp_package.clp_config.results_cache
    uri = f"mongodb://{cache.host}:{cache.port}/{cache.db_name}"
    # The package replica set advertises a hostname reachable only inside Docker.
    with MongoClient[dict[str, Any]](uri, directConnection=True) as client:
        collection = client[cache.db_name][f"limit-test-{uuid.uuid4().hex}"]
        command = [
            str(binary),
            "s",
            str(archive_paths[0]),
            "*",
            "results-cache",
            "--uri",
            uri,
            "--collection",
            collection.name,
            "--batch-size",
            "127",
        ]
        if limit is not None:
            command.extend(["--max-num-results", str(limit)])
        try:
            result = ClpAction.from_cmd(command).verify_returncode()
            assert result, result.failure_message
            expected = timestamps if limit is None else timestamps[-limit:]
            assert sorted(doc["timestamp"] for doc in collection.find()) == expected
            # Both streaming and limited output must remain idempotent on a retry.
            result = ClpAction.from_cmd(command).verify_returncode()
            assert result, result.failure_message
            assert collection.count_documents({}) == len(expected)
        finally:
            collection.drop()
