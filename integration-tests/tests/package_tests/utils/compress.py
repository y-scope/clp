"""Classes to facilitate CLP package compression testing."""

from pathlib import Path

from tests.utils.classes import CmdArgs


class CompressArgs(CmdArgs):
    """Command argument model for compressing with the CLP package."""

    script_path: Path
    config: Path
    dataset: str | None = None
    timestamp_key: str | None = None
    unstructured: bool = False
    paths: list[Path]

    def to_cmd(self) -> list[str]:
        """Converts the model attributes to a command list."""
        cmd: list[str] = [
            str(self.script_path),
            "--config",
            str(self.config),
        ]

        if self.dataset:
            cmd.append("--dataset")
            cmd.append(self.dataset)
        if self.timestamp_key:
            cmd.append("--timestamp-key")
            cmd.append(self.timestamp_key)
        if self.unstructured:
            cmd.append("--unstructured")

        cmd.extend([str(path) for path in self.paths])

        return cmd
