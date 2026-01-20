"""Wrapper for locating and invoking the bundled yscope_clp_core command line binaries."""

import functools
import subprocess
import sys
from importlib.resources import files
from pathlib import Path
from typing import NoReturn


@functools.cache
def _get_clp_exe(name: str) -> Path:
    """
    Locates a bundled CLP core executable installed with the Python distribution.

    Executables are installed into the package at build time by CMake under `yscope_clp_core/bin`.
    This helper resolves the on disk path at runtime and caches the result to avoid repeated
    filesystem lookups.

    :param name: Name of the executable to locate.
    :return: Path to the executable.
    :raise FileNotFoundError: If the executable cannot be found.
    """
    exe = Path(str(files("yscope_clp_core") / f"bin/{name}"))
    if exe.exists():
        return exe
    err_msg = f"No executable found for {name} at {exe}."
    raise FileNotFoundError(err_msg)


def _run_clp_exe(name: str, args: list[str]) -> int:
    """
    Invokes a bundled CLP core executable as a subprocess.

    :param name: Name of the executable to run.
    :param args: Arguments to pass to the executable.
    :return: The subprocess exit code.
    """
    command = [str(_get_clp_exe(name)), *args]
    return subprocess.call(command)


def clp_s() -> NoReturn:
    """Entry point that dispatches to the clp_s executable."""
    sys.exit(_run_clp_exe("clp-s", sys.argv[1:]))
