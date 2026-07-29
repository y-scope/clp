"""Compression verification helpers specific to the clp-text package."""

from tests.package_tests.classes import ClpPackage
from tests.package_tests.utils.compress import CompressArgs
from tests.package_tests.utils.decompress import DecompressArgs
from tests.utils.classes import ClpAction, ClpVerificationResult, SampleDataset
from tests.utils.fs_validation import is_dir_tree_content_equal
from tests.utils.utils import clear_directory


def verify_compress_clp_text(
    action: ClpAction,
    clp_package: ClpPackage,
    original_dataset: SampleDataset,
) -> ClpVerificationResult:
    """
    Verifies that the clp-text `clp_package` has compressed `original_dataset` correctly by
    decompressing the compressed logs and comparing the decompressed logs to the original dataset.

    :param action:
    :param clp_package:
    :param original_dataset:
    :return: A `ClpVerificationResult` indicating whether the round-trip matched the original logs.
    """
    if not isinstance(action.args, CompressArgs):
        err_msg = "Verification expects a 'CompressArgs' action."
        raise TypeError(err_msg)

    path_config = clp_package.path_config
    clear_directory(path_config.package_decompression_dir)

    args: DecompressArgs = DecompressArgs(
        script_path=path_config.decompress_path,
        config=clp_package.temp_config_file_path,
        extraction_dir=path_config.package_decompression_dir,
    )

    decompress_action = ClpAction.from_args(args)

    result = decompress_action.verify_returncode()
    if not result:
        return action.fail_verification(
            "During compress action verification, supporting call to 'decompress.sh' returned a"
            " non-zero exit code.",
            supporting_action=decompress_action,
        )

    # Verify equality between original logs and decompressed logs.
    original_logs_path = original_dataset.logs_path
    decompressed_logs_path = path_config.package_decompression_dir / original_logs_path.relative_to(
        original_logs_path.anchor
    )
    if not is_dir_tree_content_equal(original_logs_path, decompressed_logs_path):
        return action.fail_verification(
            f"Compress verification failure: mismatch between original logs at"
            f" '{original_logs_path}' and decompressed logs at '{decompressed_logs_path}'.",
            supporting_action=decompress_action,
        )

    clear_directory(path_config.package_decompression_dir)
    return action.pass_verification()
