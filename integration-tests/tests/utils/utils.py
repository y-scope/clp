"""Provide general utility functions used across `integration-tests`."""

import os
import shutil
import subprocess
from pathlib import Path
from tempfile import NamedTemporaryFile
from typing import IO


def get_env_var(var_name: str) -> str:
    """
    :param var_name:
    :return: The string value of the specified environment variable.
    :raise: ValueError if the environment variable is not set
    """
    value = os.environ.get(var_name)
    if value is None:
        err_msg = f"Environment variable {var_name} is not set."
        raise ValueError(err_msg)
    return value


def is_dir_tree_content_equal(path1: Path, path2: Path) -> bool:
    """
    :param path1:
    :param path2:
    :return: Whether two files/directories hold the exactly same content.
    :raise: RuntimeError if the diff command fails due to execution errors.
    """
    cmd = ["diff", "--brief", "--recursive", str(path1), str(path2)]
    proc = subprocess.run(cmd, check=False, capture_output=True)
    if proc.returncode == 0:
        return True
    if proc.returncode == 1:
        return False
    err_msg = f"Command failed {' '.join(cmd)}: {proc.stderr.decode()}"
    raise RuntimeError(err_msg)


def is_json_file_structurally_equal(
    json_fp1: Path,
    json_fp2: Path,
    respect_key_order: bool = False,
    respect_row_order: bool = False,
) -> bool:
    """
    :param json_fp1:
    :param json_fp2:
    :param respect_key_order: If True, preserve the original key order within each mapping (row).
    :param respect_row_order: If True, preserve the original ordering of top-level mappings.
    :return: Whether the two JSON files are structurally equivalent after being normalized with the
    same key and row ordering rules.
    """
    with (
        _sort_json_keys_and_rows(json_fp1, respect_key_order, respect_row_order) as temp_file_1,
        _sort_json_keys_and_rows(json_fp2, respect_key_order, respect_row_order) as temp_file_2,
    ):
        return is_dir_tree_content_equal(Path(temp_file_1.name), Path(temp_file_2.name))


def resolve_path_env_var(var_name: str) -> Path:
    """
    :param var_name: Name of the environment variable holding a path.
    :return: Absolute Path resolved from the input variable.
    :raise: Propagates `get_env_var`'s exceptions.
    :raise: Propagates `Path.expanduser`'s exceptions.
    :raise: Propagates `Path.resolve`'s exceptions.
    """
    return Path(get_env_var(var_name)).expanduser().resolve()


def unlink(rm_path: Path, force: bool = True) -> None:
    """
    Remove a file or directory at `path`.

    :param rm_path:
    :param force: Whether to force remove with sudo priviledges in case the normal operation fails.
                  Defaults to True.
    """
    try:
        shutil.rmtree(rm_path)
    except FileNotFoundError:
        pass
    except PermissionError:
        if not force:
            raise

        sudo_rm_cmds = ["sudo", "rm", "-rf", str(rm_path)]
        try:
            subprocess.run(sudo_rm_cmds, check=True)
        except subprocess.CalledProcessError as e:
            err_msg = f"Failed to remove {rm_path} due to lack of superuser privileges (sudo)."
            raise OSError(err_msg) from e


def validate_dir_exists(dir_path: Path) -> None:
    """
    :param dir_path:
    :raise: ValueError if the path does not exist or is not a directory.
    """
    if not dir_path.is_dir():
        err_msg = f"Path does not exist or is not a directory: {dir_path}"
        raise ValueError(err_msg)


def _sort_json_keys_and_rows(
    json_fp: Path,
    respect_key_order: bool = False,
    respect_row_order: bool = False,
) -> IO[str]:
    """
    Normalize a JSON file into a structure-insensitive representation to for content comparison.

    :param json_fp: Path to the JSON file to normalize.
    :param respect_key_order: If True, preserve the original key order within each mapping (row).
    :param respect_row_order: If True, preserve the original ordering of top-level mappings.
    :return: A named temporary file (delete on close) that contains the sorted JSON content.
    :raise: RuntimeError if either jq or sort is missing or fails due to execution errors.
    """
    jq_bin = shutil.which("jq")
    sort_bin = shutil.which("sort")
    if jq_bin is None or sort_bin is None:
        err_msg = "jq/sort executable not found"
        raise RuntimeError(err_msg)

    sorted_fp = NamedTemporaryFile(mode="w+", delete=True)  # noqa: SIM115

    jq_cmd = [jq_bin, "--compact-output"]
    if not respect_key_order:
        jq_cmd.append("--sort-keys")
    jq_cmd.append(".")  # Identity filter
    jq_cmd.append(str(json_fp))

    jq_proc = None
    try:
        if respect_row_order:
            subprocess.run(jq_cmd, stdout=sorted_fp, check=True)
        else:
            jq_proc = subprocess.Popen(jq_cmd, stdout=subprocess.PIPE)
            subprocess.run([sort_bin], stdin=jq_proc.stdout, stdout=sorted_fp, check=True)
            jq_rc = jq_proc.wait()
            if jq_rc != 0:
                raise subprocess.CalledProcessError(jq_rc, jq_cmd)  # noqa: TRY301
    except subprocess.CalledProcessError as e:
        err_msg = f"Failed to deterministically sort `{json_fp}`."
        raise RuntimeError(err_msg) from e
    finally:
        if jq_proc is not None and jq_proc.stdout is not None:
            jq_proc.stdout.close()

    sorted_fp.flush()
    sorted_fp.seek(0)
    return sorted_fp
