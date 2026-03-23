"""Classes used in CLP binary integration tests."""

import logging
from dataclasses import dataclass
from pathlib import Path

from tests.utils.classes import IntegrationTestPathConfig, static_path
from tests.utils.utils import (
    validate_dir_exists,
)

logger = logging.getLogger(__name__)


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
        super().__post_init__()

        # Validate that init directory exists.
        validate_dir_exists(self.clp_core_bins_dir)

        # Validate all static path properties.
        self.validate_static_paths()

    @property
    @static_path
    def clp_binary_path(self) -> Path:
        """:return: The absolute path to the `clp` binary."""
        return self.clp_core_bins_dir / "clp"

    @property
    @static_path
    def clp_s_binary_path(self) -> Path:
        """:return: The absolute path to the `clp-s` binary."""
        return self.clp_core_bins_dir / "clp-s"
