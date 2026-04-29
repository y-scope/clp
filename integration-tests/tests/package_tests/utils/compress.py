"""Functions to facilitate CLP package compression testing."""

import logging
from pathlib import Path

import pytest
from clp_py_utils.clp_config import StorageEngine

from tests.package_tests.classes import ClpPackage
from tests.package_tests.utils.decompress import decompress_clp_package
from tests.utils.classes import (
    CmdArgs,
    ExternalAction,
    IntegrationTestDataset,
    VerificationResult,
)
from tests.utils.logging_utils import format_action_failure_msg
from tests.utils.utils import clear_directory, is_dir_tree_content_equal

logger = logging.getLogger(__name__)


class CompressArgs(CmdArgs):
    """Command argument model for compressing with the CLP package."""

    script_path: Path
    config: Path
    dataset: str | None = None
    timestamp_key: str | None = None
    unstructured: bool = False
    paths: list[Path]

    def to_cmd(self) -> list[str]:
        """Converts the model attributes to a command list."""
        cmd: list[str] = [
            str(self.script_path),
            "--config",
            str(self.config),
        ]

        if self.dataset:
            cmd.append("--dataset")
            cmd.append(self.dataset)
        if self.timestamp_key:
            cmd.append("--timestamp-key")
            cmd.append(self.timestamp_key)
        if self.unstructured:
            cmd.append("--unstructured")

        cmd.extend([str(path) for path in self.paths])

        return cmd


def compress_clp_package(
    clp_package: ClpPackage,
    dataset: IntegrationTestDataset,
) -> ExternalAction:
    """
    Compresses the specified dataset into a CLP package.

    :param clp_package:
    :param dataset:
    :return: The `ExternalAction` instance that runs the compression.
    """
    logger.info(
        "Compressing the '%s' dataset with the '%s' package.",
        dataset.dataset_name,
        clp_package.mode_name,
    )

    args: CompressArgs = _construct_compress_args(clp_package, dataset)
    return ExternalAction(cmd=args.to_cmd(), args=args)


def _construct_compress_args(
    clp_package: ClpPackage, dataset: IntegrationTestDataset
) -> CompressArgs:
    """Construct the `CompressArgs` object for compressing the specified dataset."""
    path_config = clp_package.path_config
    args = CompressArgs(
        script_path=path_config.compress_path,
        config=clp_package.temp_config_file_path,
        paths=[dataset.logs_path],
    )

    if clp_package.clp_config.package.storage_engine == StorageEngine.CLP_S:
        args.dataset = dataset.metadata.dataset_name
        args.timestamp_key = dataset.metadata.timestamp_key
        args.unstructured = dataset.metadata.unstructured

    return args


def verify_compress_action(
    compress_action: ExternalAction,
    clp_package: ClpPackage,
    original_dataset: IntegrationTestDataset,
) -> VerificationResult:
    """
    Verifies the compression action.

    :param compress_action:
    :param clp_package:
    :param original_dataset:
    :return: A `VerificationResult` indicating the success or failure of the verification.
    """
    logger.info("Verifying %s package compression.", clp_package.mode_name)
    if compress_action.completed_proc.returncode != 0:
        return VerificationResult.fail(
            format_action_failure_msg(
                "The compress.sh subprocess returned a non-zero exit code.",
                compress_action,
            )
        )

    if original_dataset.metadata.unstructured:
        return _verify_compress_action_unstructured_logs(
            compress_action, clp_package, original_dataset
        )
    return _verify_compress_action_structured_logs(compress_action, clp_package)


def _verify_compress_action_structured_logs(
    compress_action: ExternalAction, clp_package: ClpPackage
) -> VerificationResult:
    """Verifies the compression of structured logs."""
    logger.info("Verifying %s package compression of structured logs.", clp_package.mode_name)
    if compress_action.completed_proc.returncode != 0:
        return VerificationResult.fail(
            format_action_failure_msg(
                "The compress.sh subprocess returned a non-zero exit code.",
                compress_action,
            )
        )

    # TODO: Waiting for PR 1299 (clp-json decompression) to be merged.
    return VerificationResult.ok()


def _verify_compress_action_unstructured_logs(
    compress_action: ExternalAction,
    clp_package: ClpPackage,
    original_dataset: IntegrationTestDataset,
) -> VerificationResult:
    """Verifies the compression of unstructured logs."""
    logger.info("Verifying %s package compression of unstructured logs.", clp_package.mode_name)
    if compress_action.completed_proc.returncode != 0:
        return VerificationResult.fail(
            format_action_failure_msg(
                "The 'compress.sh' subprocess returned a non-zero exit code.",
                compress_action,
            )
        )

    # Decompress the contents of `clp-package/var/data/archives`.
    path_config = clp_package.path_config
    clear_directory(path_config.package_decompression_dir)

    decompress_action = decompress_clp_package(clp_package, path_config.package_decompression_dir)
    if decompress_action.completed_proc.returncode != 0:
        pytest.fail(
            "During compress action verification, supporting call to 'decompress.sh' returned a"
            f" non-zero exit code. Subprocess log: {decompress_action.log_file_path}"
        )

    # Verify equality between original logs and decompressed logs.
    original_logs_path = original_dataset.logs_path
    decompressed_logs_path = path_config.package_decompression_dir / original_logs_path.relative_to(
        original_logs_path.anchor
    )

    equal = is_dir_tree_content_equal(original_logs_path, decompressed_logs_path)
    clear_directory(path_config.package_decompression_dir)
    if equal:
        return VerificationResult.ok()

    return VerificationResult.fail(
        format_action_failure_msg(
            f"Compress verification failure: mismatch between original logs at"
            f" '{original_logs_path}' and decompressed logs at '{decompressed_logs_path}'.",
            compress_action,
        )
    )
