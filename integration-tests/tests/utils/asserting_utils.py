"""Utilities that raise pytest assertions on failure."""

import logging
from pathlib import Path

from clp_package_utils.general import EXTRACT_FILE_CMD

from tests.package_tests.classes import ClpPackage
from tests.utils.classes import ClpAction, ClpVerificationResult
from tests.utils.fs_validation import is_dir_tree_content_equal
from tests.utils.utils import clear_directory

logger = logging.getLogger(__name__)


def verify_package_compression(
    path_to_original_dataset: Path,
    clp_package: ClpPackage,
) -> ClpVerificationResult:
    """
    Verify that compression has been executed correctly by decompressing the contents of the
    package archives directory and comparing the decompressed logs to the originals stored at
    `path_to_original_dataset`.

    :param path_to_original_dataset:
    :param clp_package:
    :return: A `ClpVerificationResult` describing the outcome.
    """
    mode = clp_package.mode_name
    log_msg = f"Verifying {mode} package compression."
    logger.info(log_msg)

    if mode == "clp-json":
        # TODO: Waiting for PR 1299 to be merged.
        return ClpVerificationResult(success=True)
    if mode == "clp-text":
        # Decompress the contents of `clp-package/var/data/archives`.
        path_config = clp_package.path_config
        decompression_dir = path_config.package_decompression_dir
        temp_config_file_path = clp_package.temp_config_file_path

        clear_directory(decompression_dir)

        decompress_cmd = [
            str(path_config.decompress_path),
            "--config",
            str(temp_config_file_path),
            EXTRACT_FILE_CMD,
            "--extraction-dir",
            str(decompression_dir),
        ]

        # Run decompression command and check that it succeeds.
        decompress_action = ClpAction.from_cmd(decompress_cmd)
        decompress_result = decompress_action.verify_returncode()
        if not decompress_result:
            clear_directory(decompression_dir)
            return decompress_result

        # Verify content equality.
        output_path = decompression_dir / path_to_original_dataset.relative_to(
            path_to_original_dataset.anchor
        )

        try:
            if not is_dir_tree_content_equal(path_to_original_dataset, output_path):
                return decompress_action.fail_verification(
                    f"Mismatch between clp input {path_to_original_dataset} and output"
                    f" {output_path}.",
                )
        finally:
            clear_directory(decompression_dir)

    return ClpVerificationResult(success=True)
