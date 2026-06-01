"""Compression verification helpers specific to the clp-json package."""

from tests.package_tests.classes import ClpPackage
from tests.package_tests.utils.compress import CompressArgs
from tests.utils.classes import ClpAction, ClpVerificationResult, SampleDataset


def verify_compress_structured_clp_json(
    action: ClpAction,
    clp_package: ClpPackage,  # noqa: ARG001
    original_dataset: SampleDataset,
) -> ClpVerificationResult:
    """
    Verifies that the clp-json `clp_package` has compressed `original_dataset` correctly by
    decompressing the compressed logs and comparing the decompressed logs to the original dataset.

    :param action:
    :param clp_package:
    :param original_dataset:
    :return: A `ClpVerificationResult` indicating whether the round-trip matched the original logs.
    """
    if not isinstance(action.args, CompressArgs):
        err_msg = "'verify_compress_structured_clp_json' requires a 'CompressArgs' action."
        raise TypeError(err_msg)
    if action.args.unstructured or original_dataset.metadata.unstructured:
        err_msg = "'verify_compress_structured_clp_json' cannot verify unstructured datasets."
        raise ValueError(err_msg)

    # TODO: Waiting for PR 1299 (clp-json decompression) to be merged.
    return action.verify_returncode()


# TODO: add verify_compress_unstructured_clp_json() and verify_compress_ir_clp_json().
