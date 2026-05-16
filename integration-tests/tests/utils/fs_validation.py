"""File structure validators."""

from pathlib import Path
from tempfile import NamedTemporaryFile
from typing import IO

from tests.utils.classes import NonClpAction
from tests.utils.utils import get_binary_path


def is_dir_tree_content_equal(path1: Path, path2: Path) -> bool:
    """
    :param path1:
    :param path2:
    :return: Whether two files/directories hold the exactly same content.
    :raise: RuntimeError if the diff command fails due to execution errors.
    """
    cmd = [get_binary_path("diff"), "--brief", "--recursive", str(path1), str(path2)]
    diff_action = NonClpAction(cmd=cmd)
    diff_action.check_returncode(good_returncodes=(0, 1))
    return diff_action.completed_proc.returncode == 0


def is_json_file_structurally_equal(json_fp1: Path, json_fp2: Path) -> bool:
    """
    :param json_fp1:
    :param json_fp2:
    :return: Whether two JSON files are structurally equal after sorting has been applied.
    """
    with (
        _sort_json_keys_and_rows(json_fp1) as temp_file_1,
        _sort_json_keys_and_rows(json_fp2) as temp_file_2,
    ):
        return is_dir_tree_content_equal(Path(temp_file_1.name), Path(temp_file_2.name))


def _sort_json_keys_and_rows(json_fp: Path) -> IO[str]:
    """
    Normalize a JSON file to a stable, deterministically ordered form for comparison.

    :param json_fp:
    :return: A named temporary file (delete on close) that contains the sorted JSON content.
    :raise: RuntimeError if jq is missing or fails due to execution errors.
    """
    jq_action = NonClpAction(
        cmd=[get_binary_path("jq"), "--sort-keys", "--compact-output", ".", str(json_fp)],
    )
    jq_action.check_returncode()

    sorted_fp = NamedTemporaryFile(mode="w+")  # noqa: SIM115
    sorted_lines = sorted(jq_action.completed_proc.stdout.splitlines())
    for line in sorted_lines:
        sorted_fp.write(f"{line}\n")
    sorted_fp.flush()
    sorted_fp.seek(0)
    return sorted_fp
