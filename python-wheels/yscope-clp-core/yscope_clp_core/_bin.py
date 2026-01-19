"""Wrapper for locating and invoking the bundled yscope_clp_core command line binaries."""

import functools
import subprocess
import sys
from importlib.resources import files
from pathlib import Path
from typing import Any


@functools.cache
def _get_clp_exe(name: str) -> Path:
    """
    Locates a bundled CLP core executable installed with the Python distribution.

    Executables are installed into the package at build time by CMake under `yscope_clp_core/bin`.
    This helper resolves the on disk path at runtime and caches the result to avoid repeated
    filesystem lookups.

    :param name: Name of the executable to locate.
    :return: Path to the executable.
    """
    exe = Path(str(files("yscope_clp_core") / f"bin/{name}"))
    if exe.exists():
        return exe
    err_msg = f"No executable found for {name} at {exe}."
    raise FileNotFoundError(err_msg)


def _run_clp_exe(name: str, *args: Any) -> int:
    """
    Invokes a bundled CLP core executable as a subprocess.

    If no arguments are provided, forwards the current process command line arguments.

    :param name: Name of the executable to run.
    :param args: Arguments to pass to the executable.
    :return: The subprocess exit code.
    """
    command = [str(_get_clp_exe(name))]
    if args:
        command += list(args)
    else:
        command += sys.argv[1:]
    return subprocess.call(command)


def clp_s() -> None:
    """Entry point that dispatches to the clp_s executable."""
    raise SystemExit(_run_clp_exe("clp-s"))
