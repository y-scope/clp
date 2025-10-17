"""Define all python classes used in `integration-tests`."""

from __future__ import annotations

import re
import socket
from dataclasses import dataclass, field, InitVar
from pathlib import Path
from typing import Any, Literal

import yaml

from tests.utils.utils import (
    unlink,
    validate_dir_exists,
    validate_file_exists,
)


@dataclass(frozen=True)
class CoreConfig:
    """The configuration for the clp core binaries subject to testing."""

    #:
    clp_core_bins_dir: Path

    def __post_init__(self) -> None:
        """
        Validates that the CLP core binaries directory exists and contains all required
        executables.
        """
        clp_core_bins_dir = self.clp_core_bins_dir
        validate_dir_exists(clp_core_bins_dir)

        # Check for required CLP core binaries
        required_binaries = ["clg", "clo", "clp", "clp-s", "indexer", "reducer-server"]
        missing_binaries = [b for b in required_binaries if not (clp_core_bins_dir / b).is_file()]
        if len(missing_binaries) > 0:
            err_msg = (
                f"CLP core binaries at {clp_core_bins_dir} are incomplete."
                f" Missing binaries: {', '.join(missing_binaries)}"
            )
            raise ValueError(err_msg)

    @property
    def clp_binary_path(self) -> Path:
        """:return: The absolute path to the core binary `clp`."""
        return self.clp_core_bins_dir / "clp"

    @property
    def clp_s_binary_path(self) -> Path:
        """:return: The absolute path to the core binary `clp-s`."""
        return self.clp_core_bins_dir / "clp-s"


@dataclass(frozen=True)
class PackageConfig:
    """The configuration for the clp package being tested."""

    #:
    clp_package_dir: Path

    clp_config_file_path: Path = field(init=False, repr=True)

    #: Hostname of the machine running the test.
    hostname: str = field(init=False, repr=True)

    def __post_init__(self) -> None:
        """Validates the values specified at init, and initialises attributes."""
        # Validate that the CLP package directory exists and contains all required directories.
        clp_package_dir = self.clp_package_dir
        validate_dir_exists(clp_package_dir)

        # Check for required package script directories
        required_dirs = ["bin", "etc", "lib", "sbin"]
        missing_dirs = [d for d in required_dirs if not (clp_package_dir / d).is_dir()]
        if len(missing_dirs) > 0:
            err_msg = (
                f"CLP package at {clp_package_dir} is incomplete."
                f" Missing directories: {', '.join(missing_dirs)}"
            )
            raise ValueError(err_msg)

        # Set clp_config_file_path and validate it.
        object.__setattr__(
            self, "clp_config_file_path", self.clp_package_dir / "etc" / "clp-config.yml"
        )
        validate_file_exists(self.clp_config_file_path)

        # Set hostname.
        object.__setattr__(self, "hostname", socket.gethostname())


@dataclass(frozen=True)
class PackageRun:
    """Metadata for the running CLP package."""

    #:
    package_config: PackageConfig

    #:
    mode: Literal["clp-text", "clp-json"]

    #:
    clp_log_dir: Path = field(init=False, repr=True)

    clp_run_config_file_path: Path = field(init=False, repr=True)

    #:
    clp_instance_id: str = field(init=False, repr=True)

    def __post_init__(self) -> None:
        """Validates the values specified at init, and initialises attributes."""
        # Set clp_log_dir after validating it.
        clp_log_dir = self.package_config.clp_package_dir / "var" / "log"
        validate_dir_exists(clp_log_dir)
        object.__setattr__(self, "clp_log_dir", clp_log_dir)

        # Set clp_run_config_file_path after validating it.
        clp_run_config_file_path = (
            self.clp_log_dir / self.package_config.hostname / ".clp-config.yml"
        )
        validate_file_exists(clp_run_config_file_path)
        object.__setattr__(self, "clp_run_config_file_path", clp_run_config_file_path)

        # Set clp_instance_id.
        clp_instance_id_file_path = self.clp_log_dir / self.package_config.hostname / "instance-id"
        validate_file_exists(clp_instance_id_file_path)
        clp_instance_id = self._get_clp_instance_id(clp_instance_id_file_path)
        object.__setattr__(self, "clp_instance_id", clp_instance_id)

        # Validate mode.
        self._assert_mode_matches_run_config()

    def _assert_mode_matches_run_config(self) -> None:
        """Validates that self.mode matches the package values in the run config file."""
        config_dict = self._load_run_config(self.clp_run_config_file_path)
        config_mode = self._extract_mode_from_config(config_dict, self.clp_run_config_file_path)

        if config_mode != self.mode:
            error_msg = (
                f"Mode mismatch: the mode specified to the PackageRun object was {self.mode},"
                f" but the package is running in {config_mode} mode."
            )
            raise ValueError(error_msg)

    @staticmethod
    def _load_run_config(path: Path) -> dict[str, Any]:
        """Load the run config file into a dictionary."""
        try:
            with path.open("r", encoding="utf-8") as file:
                config_dict = yaml.safe_load(file)
        except yaml.YAMLError as err:
            raise ValueError(f"Invalid YAML in run config {path}: {err}") from err
        except OSError as err:
            raise ValueError(f"Cannot read run config {path}: {err}") from err

        if not isinstance(config_dict, dict):
            raise TypeError(f"Run config {path} must be a mapping at the top level")

        return config_dict

    @staticmethod
    def _extract_mode_from_config(
        config_dict: dict[str, Any],
        path: Path,
    ) -> Literal["clp-text", "clp-json"]:
        """Determine the package mode from the contents of the run-config dictionary."""
        package = config_dict.get("package")
        if not isinstance(package, dict):
            raise TypeError(f"Run config {path} is missing the 'package' mapping.")

        query_engine = package.get("query_engine")
        storage_engine = package.get("storage_engine")
        if query_engine is None or storage_engine is None:
            raise ValueError(
                f"Run config {path} must specify both 'package.query_engine' and"
                " 'package.storage_engine'."
            )

        if query_engine == "clp" and storage_engine == "clp":
            return "clp-text"
        if query_engine == "clp-s" and storage_engine == "clp-s":
            return "clp-json"

        raise ValueError(
            f"Run config {path} specifies running conditions for which integration testing is not"
            f"supported: query_engine={query_engine}, storage_engine={storage_engine}."
        )

    @staticmethod
    def _get_clp_instance_id(clp_instance_id_file_path: Path) -> str:
        """Return the 4-digit hexadecimal CLP instance-id stored at clp_instance_id_file_path."""
        try:
            contents = clp_instance_id_file_path.read_text(encoding="utf-8").strip()
        except OSError as err:
            raise ValueError(
                f"Cannot read instance-id file {clp_instance_id_file_path}: {err}"
            ) from err

        if not re.fullmatch(r"[0-9a-fA-F]{4}", contents):
            raise ValueError(
                f"Invalid instance ID in {clp_instance_id_file_path}: expected a 4-character"
                f" hexadecimal string, but read {contents}."
            )

        return contents


@dataclass(frozen=True)
class IntegrationTestConfig:
    """The general configuration for integration tests."""

    #: Root directory for integration tests output.
    test_root_dir: Path
    logs_download_dir_init: InitVar[Path | None] = None
    #: Directory to store the downloaded logs.
    logs_download_dir: Path = field(init=False, repr=True)

    def __post_init__(self, logs_download_dir_init: Path | None) -> None:
        """Initialize and create required directories for integration tests."""
        if logs_download_dir_init is not None:
            object.__setattr__(self, "logs_download_dir", logs_download_dir_init)
        else:
            object.__setattr__(self, "logs_download_dir", self.test_root_dir / "downloads")

        self.test_root_dir.mkdir(parents=True, exist_ok=True)
        self.logs_download_dir.mkdir(parents=True, exist_ok=True)


@dataclass(frozen=True)
class IntegrationTestLogs:
    """Metadata for the downloaded logs used for integration tests."""

    #:
    name: str
    #:
    tarball_url: str
    integration_test_config: InitVar[IntegrationTestConfig]
    #:
    tarball_path: Path = field(init=False, repr=True)
    #:
    extraction_dir: Path = field(init=False, repr=True)

    def __post_init__(self, integration_test_config: IntegrationTestConfig) -> None:
        """Initialize and set tarball and extraction paths for integration test logs."""
        name = self.name.strip()
        if 0 == len(name):
            err_msg = "`name` cannot be empty."
            raise ValueError(err_msg)
        logs_download_dir = integration_test_config.logs_download_dir
        validate_dir_exists(logs_download_dir)

        object.__setattr__(self, "name", name)
        object.__setattr__(self, "tarball_path", logs_download_dir / f"{name}.tar.gz")
        object.__setattr__(self, "extraction_dir", logs_download_dir / name)


@dataclass(frozen=True)
class CompressionTestConfig:
    """Compression test configuration providing per-test metadata for artifacts and directories."""

    #:
    test_name: str
    #: Directory containing the original (uncompressed) log files used by this test.
    logs_source_dir: Path
    integration_test_config: InitVar[IntegrationTestConfig]
    #: Path to store compressed archives generated by the test.
    compression_dir: Path = field(init=False, repr=True)
    #: Path to store decompressed logs generated by the test.
    decompression_dir: Path = field(init=False, repr=True)

    def __post_init__(self, integration_test_config: IntegrationTestConfig) -> None:
        """Initialize and set required directory paths for compression tests."""
        test_name = self.test_name.strip()
        if 0 == len(test_name):
            err_msg = "`test_name` cannot be empty."
            raise ValueError(err_msg)
        test_root_dir = integration_test_config.test_root_dir
        validate_dir_exists(test_root_dir)

        object.__setattr__(self, "test_name", test_name)
        object.__setattr__(self, "compression_dir", test_root_dir / f"{test_name}-archives")
        object.__setattr__(
            self, "decompression_dir", test_root_dir / f"{test_name}-decompressed-logs"
        )

    def clear_test_outputs(self) -> None:
        """Remove any existing output directories created by this compression test."""
        unlink(self.compression_dir)
        unlink(self.decompression_dir)
