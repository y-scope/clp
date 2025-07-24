import os
import subprocess
from pathlib import Path
from typing import List


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
