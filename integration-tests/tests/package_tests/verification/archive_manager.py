"""Shared verification helpers for the CLP package archive-manager."""

import re

from tests.package_tests.classes import ClpPackage
from tests.package_tests.utils.archive_manager import (
    ArchiveManagerArgs,
    run_archive_manager_find,
)
from tests.utils.classes import (
    ClpAction,
    ClpVerificationResult,
    SampleDataset,
)


def verify_archive_manager_find(
    action: ClpAction,
    clp_package: ClpPackage,
    dataset: SampleDataset | None = None,
) -> ClpVerificationResult:
    """
    Verifies the archive-manager 'find' action with the following procedure:

    1. Reconstructs the full set of archives with two complementary 'find' calls (one for archives
       before `begin_ts` and one for archives after `end_ts`)
    2. Performs an unfiltered 'find' over all archives.
    3. Compares the two above sets.

    :param action:
    :param clp_package:
    :param dataset:
    :return: A `ClpVerificationResult` indicating the success or failure of the verification.
    """
    if not isinstance(action.args, ArchiveManagerArgs):
        err_msg = "Verification expects an 'ArchiveManagerArgs' action."
        raise TypeError(err_msg)

    returncode_result = action.verify_returncode()
    if not returncode_result:
        return returncode_result

    args = action.args
    begin_ts = args.begin_ts if args.begin_ts is not None else 0
    end_ts = args.end_ts
    assembled_archive_id_list: list[str] = []

    # Find archives before begin_ts.
    if begin_ts > 0:
        find_before_action = run_archive_manager_find(
            clp_package=clp_package,
            dataset=dataset,
            begin_ts=0,
            end_ts=begin_ts,
        )
        find_before_result = find_before_action.verify_returncode()
        if not find_before_result:
            return action.fail_verification(
                "During archive-manager 'find' verification, supporting call to archive-manager"
                " 'find' returned a non-zero exit code.",
                supporting_action=find_before_action,
            )
        assembled_archive_id_list.extend(extract_archive_ids_from_find_output(find_before_action))

    # Add the archives from the original command.
    assembled_archive_id_list.extend(extract_archive_ids_from_find_output(action))

    # Find archives after end_ts.
    if end_ts is not None:
        find_after_action = run_archive_manager_find(
            clp_package=clp_package,
            dataset=dataset,
            begin_ts=end_ts,
        )
        find_after_result = find_after_action.verify_returncode()
        if not find_after_result:
            return action.fail_verification(
                "During archive-manager 'find' verification, supporting call to archive-manager"
                " 'find' returned a non-zero exit code.",
                supporting_action=find_after_action,
            )
        assembled_archive_id_list.extend(extract_archive_ids_from_find_output(find_after_action))

    # Find all.
    find_all_action = run_archive_manager_find(
        clp_package=clp_package,
        dataset=dataset,
    )
    find_all_result = find_all_action.verify_returncode()
    if not find_all_result:
        return action.fail_verification(
            "During archive-manager 'find' verification, supporting call to archive-manager"
            " 'find' returned a non-zero exit code.",
            supporting_action=find_all_action,
        )
    all_archive_ids_list = extract_archive_ids_from_find_output(find_all_action)

    # Compare.
    assembled_archive_id_list.sort()
    all_archive_ids_list.sort()
    if assembled_archive_id_list == all_archive_ids_list:
        return action.pass_verification()

    return action.fail_verification(
        "Archive-manager 'find' verification failure: mismatch between the assembled archive ID"
        f" list '{assembled_archive_id_list}' and the list of all archive IDs"
        f" '{all_archive_ids_list}'",
        supporting_action=find_all_action,
    )


def verify_archive_manager_del_by_ids(
    action: ClpAction,
    clp_package: ClpPackage,
    dataset: SampleDataset | None = None,
) -> ClpVerificationResult:
    """
    Verifies the archive-manager 'del by-ids' action by querying the current set of archives with
    'find' and confirming that none of the IDs that were targeted for deletion remain.

    :param action:
    :param clp_package:
    :param dataset:
    :return: A `ClpVerificationResult` indicating the success or failure of the verification.
    """
    if not isinstance(action.args, ArchiveManagerArgs):
        err_msg = "Verification expects an 'ArchiveManagerArgs' action."
        raise TypeError(err_msg)

    returncode_result = action.verify_returncode()
    if not returncode_result:
        return returncode_result

    args = action.args
    find_all_action = run_archive_manager_find(clp_package=clp_package, dataset=dataset)
    find_result = verify_archive_manager_find(find_all_action, clp_package, dataset)
    if not find_result:
        return action.fail_verification(
            "During archive-manager 'del' verification, supporting call to archive-manager 'find'"
            f" could not be verified: '{find_result.failure_message}'",
            supporting_action=find_all_action,
        )

    current_ids = extract_archive_ids_from_find_output(find_all_action)
    if args.ids and any(item in current_ids for item in args.ids):
        return action.fail_verification(
            "Archive-manager 'del by-ids' verification failure: Some archives that were"
            " specified for deletion are still present in the metadata database.",
            supporting_action=find_all_action,
        )

    return action.pass_verification()


def verify_archive_manager_del_by_filter(
    action: ClpAction,
    clp_package: ClpPackage,
    dataset: SampleDataset | None = None,
) -> ClpVerificationResult:
    """
    Verifies the archive-manager 'del by-filter' action by running a 'find' with the same time-range
    filter that was used for deletion, and confirming that no archives remain within that range.

    :param action:
    :param clp_package:
    :param dataset:
    :return: A `ClpVerificationResult` indicating the success or failure of the verification.
    """
    if not isinstance(action.args, ArchiveManagerArgs):
        err_msg = "Verification expects an 'ArchiveManagerArgs' action."
        raise TypeError(err_msg)

    returncode_result = action.verify_returncode()
    if not returncode_result:
        return returncode_result

    args = action.args
    find_action = run_archive_manager_find(
        clp_package=clp_package,
        dataset=dataset,
        begin_ts=args.begin_ts,
        end_ts=args.end_ts,
    )
    find_result = verify_archive_manager_find(find_action, clp_package, dataset)
    if not find_result:
        return action.fail_verification(
            "During archive-manager 'del' verification, supporting call to archive-manager 'find'"
            f" could not be verified: '{find_result.failure_message}'",
            supporting_action=find_action,
        )

    current_ids = extract_archive_ids_from_find_output(find_action)
    if len(current_ids) > 0:
        return action.fail_verification(
            "Archive-manager 'del by-filter' verification failure: Some archives that"
            " should have been deleted were not deleted.",
            supporting_action=find_action,
        )

    return action.pass_verification()


def extract_archive_ids_from_find_output(
    action: ClpAction,
) -> list[str]:
    """
    Extracts archive IDs from the output of an archive-manager 'find' action.

    :param action:
    :return: A list of archive IDs extracted from the action's output.
    """
    output_archive_id_list: list[str] = []
    output_lines = action.get_output().splitlines()

    num_archive_ids = 0
    filtered_lines = []
    for line in output_lines:
        match = re.search(r"Found (\d+) archives within the specified time range", line)
        if match:
            num_archive_ids = int(match.group(1))
        else:
            filtered_lines.append(line)

    if num_archive_ids == 0:
        return output_archive_id_list

    uuid_pattern = re.compile(
        r"[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}"
    )
    for line in filtered_lines:
        match = uuid_pattern.search(line)
        if match:
            output_archive_id_list.append(match.group(0))

    return sorted(output_archive_id_list)
