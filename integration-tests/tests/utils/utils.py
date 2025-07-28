import os
import subprocess
from pathlib import Path
from tempfile import NamedTemporaryFile
from typing import IO, List


def is_json_file_structurally_equal(json_fp1: Path, json_fp2: Path) -> bool:
    with _sort_json_keys_and_rows(json_fp1) as s1, _sort_json_keys_and_rows(json_fp2) as s2:
        return is_dir_tree_content_equal(s1.name, s2.name)


def is_dir_tree_content_equal(path1: Path, path2: Path) -> bool:
    cmd = ["diff", "--brief", "--recursive", str(path1), str(path2)]
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if 0 == proc.returncode:
        return True
    if 1 == proc.returncode:
        return False
    raise RuntimeError(f"Command failed {' '.join(cmd)}: {proc.stderr.decode()}")


def get_env_var(var_name: str) -> str:
    value = os.environ.get(var_name)
    assert value is not None, f"Environment variable {var_name} is not set."
    return value


def run_and_assert(cmd: List[str], **kwargs) -> subprocess.CompletedProcess:
    proc = subprocess.run(cmd, **kwargs)
    assert 0 == proc.returncode, f"Command failed: {' '.join(cmd)}"
    return proc


def _sort_json_keys_and_rows(json_fp: Path) -> IO[str]:
    sorted_fp = NamedTemporaryFile(mode="w+", delete=True)
    jq_proc = subprocess.Popen(
        ["jq", "--sort-keys", "--compact-output", ".", str(json_fp)],
        stdout=subprocess.PIPE,
    )
    subprocess.run(["sort"], stdin=jq_proc.stdout, stdout=sorted_fp, check=True)
    sorted_fp.flush()
    return sorted_fp
