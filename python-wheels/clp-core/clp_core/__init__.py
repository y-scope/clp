import subprocess
import sys
from pathlib import Path
import functools

if sys.version_info.minor >= 9:
    # Only available on 3.9 or later, and required on 3.12
    from importlib.resources import files
else:
    import pkg_resources


@functools.lru_cache(maxsize=None)
def _get_executable(name:str) -> Path:
    exe: Path
    if sys.version_info.minor >= 9:
        # Only available in 3.9 or later, and required in 3.12
        exe = Path(files("clp_core") / f"bin/{name}")
    else:
        exe = Path(pkg_resources.resource_filename("clp_core", f"bin/{name}"))
    if exe.exists():
        return exe

    raise FileNotFoundError(f"No executable found for {name} at {exe}")

def _run(name, *args):
    command = [_get_executable(name)]
    if args:
        command += list(args)
    else:
        command += sys.argv[1:]
    return subprocess.call(command)

def _run_python(name, *args):
    command = [sys.executable, _get_executable(name)]
    if args:
        command += list(args)
    else:
        command += sys.argv[1:]

    # as MS Windows is not able to run Python scripts directly by name,
    # we have to call the interpreter and pass the script as parameter
    return subprocess.call(command)


def clp_s():
    raise SystemExit(_run("clp-s"))
