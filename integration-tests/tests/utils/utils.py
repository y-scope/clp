import os
import subprocess
from pathlib import Path
from tempfile import NamedTemporaryFile
from typing import Any, IO, List


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


def is_dir_tree_content_equal(path1: Path, path2: Path) -> bool:
    """
    :param path1:
    :param path2:
    :return: Whether two files/directories hold the exactly same content.
    :raise: RuntimeError if the diff command fails due to execution errors.
    """
    cmd = ["diff", "--brief", "--recursive", str(path1), str(path2)]
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if 0 == proc.returncode:
        return True
    if 1 == proc.returncode:
        return False
    raise RuntimeError(f"Command failed {' '.join(cmd)}: {proc.stderr.decode()}")


def get_env_var(var_name: str) -> str:
    """
    :param var_name:
    :return: The string value of the specified environment variable.
    :raise: ValueError if the environment variable is not set
    """
    value = os.environ.get(var_name)
    if value is None:
        raise ValueError(f"Environment variable {var_name} is not set.")
    return value


def run_and_assert(cmd: List[str], **kwargs: Any) -> subprocess.CompletedProcess[Any]:
    """
    Runs a command with subprocess and asserts that it succeeds with pytest.

    :param cmd: Command and arguments to execute.
    :param kwargs: Additional keyword arguments passed through to the subprocess.
    :return: The completed process object, for inspection or further handling.
    :raise: AssertionError if the command exits with a non-zero return code.
    """
    proc = subprocess.run(cmd, **kwargs)
    assert 0 == proc.returncode, f"Command failed: {' '.join(cmd)}"
    return proc


def validate_dir_exists(dir_path: Path) -> None:
    """
    :param dir_path:
    :raise: ValueError if the path does not exist or is not a directory.
    """
    if not dir_path.exists():
        raise ValueError(f"Directory does not exist: {dir_path}")
    if not dir_path.is_dir():
        raise ValueError(f"Path is not a directory: {dir_path}")


def _sort_json_keys_and_rows(json_fp: Path) -> IO[str]:
    """
    Normalize a JSON file to a stable, deterministically ordered form for comparison.

    :param json_fp:
    :return: A named temoprary file (delete on close) that contains the sorted JSON content.
    :raise: RuntimeError if either jq or sort fails due to execution errors.
    """
    sorted_fp = NamedTemporaryFile(mode="w+", delete=True)
    jq_proc = subprocess.Popen(
        ["jq", "--sort-keys", "--compact-output", ".", str(json_fp)],
        stdout=subprocess.PIPE,
    )
    try:
        sort_proc = subprocess.run(["sort"], stdin=jq_proc.stdout, stdout=sorted_fp, check=True)
        sort_rc = sort_proc.returncode
        if sort_rc != 0:
            raise RuntimeError(f"sort failed with exist code {sort_rc} for {json_fp}")
    finally:
        if jq_proc.stdout is not None:
            jq_proc.stdout.close()
        jq_rc = jq_proc.wait()
        if jq_rc != 0:
            raise RuntimeError(f"jq failed with exit code {jq_rc} for {json_fp}")

    sorted_fp.flush()
    sorted_fp.seek(0)
    return sorted_fp
