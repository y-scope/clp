"""Utilities for creating and manipulating archive files."""

from pathlib import Path

from tests.utils.classes import NonClpAction
from tests.utils.utils import get_binary_path


def create_tar_gz_from_dir(source_dir: Path, tar_gz_path: Path) -> None:
    """
    Create a gzip-compressed tar archive from the contents of `source_dir`.

    :param source_dir: Directory whose contents will be archived.
    :param tar_gz_path: Path where the .tar.gz archive will be written.
    :raise RuntimeError: If `tar` returns a non-zero exit code.
    """
    tar_gz_path.parent.mkdir(parents=True, exist_ok=True)

    # fmt: off
    cmd = [
        get_binary_path("tar"),
        "--create",
        "--gzip",
        "--file", str(tar_gz_path),
        "--directory", str(source_dir),
        ".",
    ]
    # fmt: on
    tar_action = NonClpAction(cmd=cmd)
    tar_action.check_returncode()
