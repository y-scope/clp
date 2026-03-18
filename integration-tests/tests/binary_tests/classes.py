"""Classes used in CLP binary integration tests."""

import argparse
import logging
from dataclasses import dataclass, field
from pathlib import Path

import pytest

from tests.utils.classes import IntegrationTestExternalAction, IntegrationTestPathConfig
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

        clp_core_bins_dir = self.clp_core_bins_dir
        validate_dir_exists(clp_core_bins_dir)
        required_binaries = [
            "clg",
            "clo",
            "clp",
            "clp-s",
            "indexer",
            "log-converter",
            "reducer-server",
        ]
        missing_binaries = [b for b in required_binaries if not (clp_core_bins_dir / b).is_file()]
        if len(missing_binaries) > 0:
            err_msg = (
                f"CLP core binaries at {clp_core_bins_dir} are incomplete. Missing binaries:"
                f" {', '.join(missing_binaries)}"
            )
            pytest.fail(err_msg)

    @property
    def clp_binary_path(self) -> Path:
        """:return: The absolute path to the `clp` binary."""
        return self.clp_core_bins_dir / "clp"

    @property
    def clp_s_binary_path(self) -> Path:
        """:return: The absolute path to the `clp-s` binary."""
        return self.clp_core_bins_dir / "clp-s"


@dataclass
class ClpBinaryExternalAction(IntegrationTestExternalAction):
    """Metadata for an external action executed during a CLP binary integration test."""

    #: Parser to define semantics for the content of `cmd`.
    args_parser: argparse.ArgumentParser

    #: Namespace to hold information from `cmd`.
    parsed_args: argparse.Namespace = field(init=False)

    def __post_init__(self) -> None:
        """Parse command arguments."""
        self.parsed_args = self.args_parser.parse_args(self.cmd[1:])
