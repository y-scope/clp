"""Classes used in CLP binary integration tests."""

from dataclasses import dataclass
from pathlib import Path

from tests.utils.classes import (
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
