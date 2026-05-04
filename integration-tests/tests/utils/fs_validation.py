"""File structure validators."""

import shutil
from pathlib import Path
from tempfile import NamedTemporaryFile
from typing import IO

from tests.utils.classes import ExternalAction


def is_dir_tree_content_equal(path1: Path, path2: Path) -> bool:
    """
    :param path1:
    :param path2:
    :return: Whether two files/directories hold the exactly same content.
    :raise: RuntimeError if the diff command fails due to execution errors.
    """
    cmd = ["diff", "--brief", "--recursive", str(path1), str(path2)]
    diff_action = ExternalAction(cmd=cmd)
    rc = diff_action.completed_proc.returncode
    if rc == 0:
        return True
    if rc == 1:
        return False
    err_msg = f"Command failed {' '.join(cmd)}: {diff_action.completed_proc.stderr}"
    raise RuntimeError(err_msg)


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
    jq_bin = shutil.which("jq")
    if jq_bin is None:
        err_msg = "jq executable not found"
        raise RuntimeError(err_msg)

    jq_action = ExternalAction(
        cmd=[jq_bin, "--sort-keys", "--compact-output", ".", str(json_fp)],
    )
    jq_rc = jq_action.completed_proc.returncode
    if jq_rc != 0:
        err_msg = f"jq failed with exit code {jq_rc} for {json_fp}"
        raise RuntimeError(err_msg)

    sorted_fp = NamedTemporaryFile(mode="w+", delete=True)  # noqa: SIM115
    sorted_lines = sorted(jq_action.completed_proc.stdout.splitlines())
    for line in sorted_lines:
        sorted_fp.write(f"{line}\n")
    sorted_fp.flush()
    sorted_fp.seek(0)
    return sorted_fp
