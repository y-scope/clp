"""Classes to facilitate CLP package dataset-manager testing."""

from pathlib import Path

from strenum import StrEnum

from tests.utils.classes import CmdArgs


class ClpPackageDatasetManagerCommand(StrEnum):
    """Commands supported by dataset-manager."""

    LIST_COMMAND = "list"
    DEL_COMMAND = "del"


class DatasetManagerArgs(CmdArgs):
    """Command argument model for dataset-manager."""

    script_path: Path
    config: Path
    command: str
    del_all: bool = False
    datasets: list[str] | None = None

    def to_cmd(self) -> list[str]:
        """Converts the model attributes to a command list."""
        cmd = [str(self.script_path), "--config", str(self.config)]

        cmd.append(self.command)

        if self.del_all:
            cmd.append("--all")
        if self.datasets:
            cmd.extend(self.datasets)

        return cmd
