"""Wrapper for locating and invoking the bundled clp_core command line binaries."""

import functools
import subprocess
import sys
from importlib.resources import files
from pathlib import Path
from typing import Any


@functools.cache
def _get_executable(name: str) -> Path:
    exe = Path(str(files("clp_core") / f"bin/{name}"))
    if exe.exists():
        return exe
    err_msg = f"No executable found for {name} at {exe}"
    raise FileNotFoundError(err_msg)


def _run(name: str, *args: Any) -> int:
    command = [str(_get_executable(name))]
    if args:
        command += list(args)
    else:
        command += sys.argv[1:]
    return subprocess.call(command)


def clp_s() -> None:
    """Entry point that dispatches to the clp_s executable."""
    raise SystemExit(_run("clp-s"))
