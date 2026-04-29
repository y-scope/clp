"""Functions to facilitate CLP package decompression testing."""

import logging
from pathlib import Path
from typing import Any

from clp_package_utils.general import EXTRACT_FILE_CMD

from tests.package_tests.classes import ClpPackage
from tests.utils.classes import CmdArgs, ExternalAction

logger = logging.getLogger(__name__)


class DecompressArgs(CmdArgs):
    """Command argument model for decompressing with the CLP package."""

    script_path: Path
    config: Path
    extraction_dir: Path
    paths: list[Path] | None = None

    def to_cmd(self) -> list[str]:
        """Converts the model attributes to a command list."""
        cmd: list[str] = [
            str(self.script_path),
            "--config",
            str(self.config),
            EXTRACT_FILE_CMD,
            "--extraction-dir",
            str(self.extraction_dir),
        ]

        if self.paths:
            cmd.extend([str(path) for path in self.paths])

        return cmd


def decompress_clp_package(
    clp_package: ClpPackage,
    extraction_dir: Any,
    paths: list[Path] | None = None,
) -> ExternalAction:
    """
    Decompresses the specified CLP package archives. Note that decompression can only be used in
    conjunction with compression.

    :param clp_package:
    :param extraction_dir:
    :param paths:
    :return: The `ExternalAction` instance that runs the decompression.
    """
    logger.info("Decompressing the '%s' package archives.", clp_package.mode_name)

    args: DecompressArgs = _construct_decompress_args(clp_package, extraction_dir, paths)
    return ExternalAction(cmd=args.to_cmd(), args=args)


def _construct_decompress_args(
    clp_package: ClpPackage,
    extraction_dir: Any,
    paths: list[Path] | None = None,
) -> DecompressArgs:
    """Constructs the `DecompressArgs` object for decompressing the specified CLP package."""
    path_config = clp_package.path_config
    args = DecompressArgs(
        script_path=path_config.decompress_path,
        config=clp_package.temp_config_file_path,
        extraction_dir=extraction_dir,
    )

    if paths:
        args.paths = paths

    return args
