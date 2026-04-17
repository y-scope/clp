"""Classes used in CLP binary integration tests."""

from dataclasses import dataclass, field
from pathlib import Path

from pydantic import BaseModel

from tests.utils.classes import (
    IntegrationTestExternalAction,
    IntegrationTestPathConfig,
)
from tests.utils.utils import (
    validate_dir_exists,
)


@dataclass
class ClpBinaryTestPathConfig(IntegrationTestPathConfig):
    """Path configuration for CLP binary integration tests."""

    #: Default CLP binary directory.
    clp_core_bins_dir: Path

    def __post_init__(self) -> None:
        """
        Validates that the CLP core binaries directory exists and contains all required
        executables.
        """
        # Validate that init directory exists.
        validate_dir_exists(self.clp_core_bins_dir)

        super().__post_init__()

    @property
    def clp_binary_path(self) -> Path:
        """:return: The absolute path to the `clp` binary."""
        return self.clp_core_bins_dir / "clp"

    @property
    def clp_s_binary_path(self) -> Path:
        """:return: The absolute path to the `clp-s` binary."""
        return self.clp_core_bins_dir / "clp-s"

    def _static_paths(self) -> list[Path]:
        """:return: Paths that must exist on disk at construction time."""
        return [
            *super()._static_paths(),
            self.clp_binary_path,
            self.clp_s_binary_path,
        ]


class ClpBinaryCmdArgs(BaseModel):
    """Base class for all CLP binary command argument models."""

    binary_path: Path

    def to_cmd(self) -> list[str]:
        """
        When overriding `to_cmd()` in derived classes, `super().to_cmd()` should be called.

        :return: list of command arguments.
        """
        return [
            str(self.binary_path),
        ]


@dataclass
class ClpBinaryExternalAction(IntegrationTestExternalAction):
    """Metadata for an external action executed during a CLP binary integration test."""

    #: Pydantic object storing semantic info required to construct `cmd` and verify the Action.
    args: ClpBinaryCmdArgs

    #: Overridden from `IntegrationTestExternalAction`. Constructed in `__post_init__` using `args`.
    cmd: list[str] = field(init=False)

    def __post_init__(self) -> None:
        """Constructs `cmd` using `args`."""
        self.cmd = self.args.to_cmd()
