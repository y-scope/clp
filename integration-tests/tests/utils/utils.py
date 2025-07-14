import os
import subprocess
from typing import List


def get_env_var(var_name: str) -> str:
    value = os.environ.get(var_name)
    assert value is not None, f"Environment variable {var_name} is not set."
    return value


def run_and_assert(cmd: List[str], **kwargs) -> subprocess.CompletedProcess:
    proc = subprocess.run(cmd, **kwargs)
    assert 0 == proc.returncode, f"Command failed: {' '.join(cmd)}"
    return proc
