"""Classes to facilitate CLP package decompression testing."""

from pathlib import Path

from clp_package_utils.general import EXTRACT_FILE_CMD

from tests.utils.classes import CmdArgs


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
