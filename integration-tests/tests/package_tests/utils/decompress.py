"""Functions to facilitate CLP package decompression testing."""

import logging
from pathlib import Path
from typing import Any

from clp_package_utils.general import EXTRACT_FILE_CMD

from tests.package_tests.classes import ClpPackage
from tests.utils.classes import ClpAction, CmdArgs

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
) -> ClpAction:
    """
    Decompresses the specified CLP package archives. Note that decompression can only be used in
    conjunction with compression.

    :param clp_package:
    :param extraction_dir:
    :param paths:
    :return: The `ClpAction` instance that runs the decompression.
    """
    logger.info("Decompressing '%s' package.", clp_package.mode_name)

    path_config = clp_package.path_config
    args: DecompressArgs = DecompressArgs(
        script_path=path_config.decompress_path,
        config=clp_package.temp_config_file_path,
        extraction_dir=extraction_dir,
        paths=paths,
    )
    return ClpAction.from_args(args)
