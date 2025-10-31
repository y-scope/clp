"""Define all python classes used in `integration-tests`."""

from __future__ import annotations

import re
from dataclasses import dataclass, field, InitVar
from pathlib import Path
from typing import Any

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
    """Metadata for the clp package test on this system."""

    #: The directory the package is located in.
    clp_package_dir: Path

    #: Root directory for package tests output.
    test_root_dir: Path

    temp_config_dir_init: InitVar[Path | None] = None
    #: Directory to store any cached package config files.
    temp_config_dir: Path = field(init=False, repr=True)

    def __post_init__(self, temp_config_dir_init: Path | None) -> None:
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

        # Initialize and create required cache directory for package tests.
        if temp_config_dir_init is not None:
            object.__setattr__(self, "temp_config_dir", temp_config_dir_init)
        else:
            object.__setattr__(self, "temp_config_dir", self.test_root_dir / "config-cache")

        self.test_root_dir.mkdir(parents=True, exist_ok=True)
        self.temp_config_dir.mkdir(parents=True, exist_ok=True)

    @property
    def start_script_path(self) -> Path:
        """:return: The absolute path to the package start script."""
        return self.clp_package_dir / "sbin" / "start-clp.sh"

    @property
    def stop_script_path(self) -> Path:
        """:return: The absolute path to the package stop script."""
        return self.clp_package_dir / "sbin" / "stop-clp.sh"


@dataclass(frozen=True)
class PackageInstanceConfigFile:
    """Metadata for the clp-config.yml file used to configure a clp package instance."""

    #: The PackageConfig object corresponding to this package run.
    package_config: PackageConfig

    #: The mode name of this config file's configuration.
    mode: str

    #: The location of the configfile used during this package run.
    temp_config_file_path: Path = field(init=False, repr=True)

    #: The path to the original pre-test clp-config.yml file.
    original_config_file_path: Path = field(init=False, repr=True)

    def __post_init__(self) -> None:
        """Validates the values specified at init, and initialises attributes."""
        # Set original_config_file_path and validate it.
        object.__setattr__(
            self,
            "original_config_file_path",
            self.package_config.clp_package_dir / "etc" / "clp-config.yml",
        )
        validate_file_exists(self.original_config_file_path)


@dataclass(frozen=True)
class PackageInstance:
    """Metadata for a run of the clp package."""

    #:
    package_instance_config_file: PackageInstanceConfigFile

    #:
    clp_log_dir: Path = field(init=False, repr=True)

    #: The path to the .clp-config.yml file constructed by the package during spin-up.
    dot_config_file_path: Path = field(init=False, repr=True)

    #:
    clp_instance_id: str = field(init=False, repr=True)

    def __post_init__(self) -> None:
        """Validates the values specified at init, and initialises attributes."""
        # Set clp_log_dir and validate that it exists.
        clp_log_dir = (
            self.package_instance_config_file.package_config.clp_package_dir / "var" / "log"
        )
        validate_dir_exists(clp_log_dir)
        object.__setattr__(self, "clp_log_dir", clp_log_dir)

        # Set dot_config_file_path after validating it.
        dot_config_file_path = self.clp_log_dir / ".clp-config.yml"
        validate_file_exists(dot_config_file_path)
        object.__setattr__(self, "dot_config_file_path", dot_config_file_path)

        # Set clp_instance_id.
        clp_instance_id_file_path = self.clp_log_dir / "instance-id"
        validate_file_exists(clp_instance_id_file_path)
        clp_instance_id = self._get_clp_instance_id(clp_instance_id_file_path)
        object.__setattr__(self, "clp_instance_id", clp_instance_id)

        # Sanity check: validate that the package is running in the correct mode.
        running_mode = self._get_running_mode()
        intended_mode = self.package_instance_config_file.mode
        if running_mode != intended_mode:
            err_msg = (
                f"Mode mismatch: the package is running in {running_mode},"
                f" but it should be running in {intended_mode}."
            )
            raise ValueError(err_msg)

    def _get_running_mode(self) -> str:
        """Gets the current running mode of the clp package."""
        config_dict = self._load_dot_config(self.dot_config_file_path)
        return self._extract_mode_from_dot_config(config_dict, self.dot_config_file_path)

    @staticmethod
    def _load_dot_config(path: Path) -> dict[str, Any]:
        """Load the run config file into a dictionary."""
        try:
            with path.open("r", encoding="utf-8") as file:
                config_dict = yaml.safe_load(file)
        except yaml.YAMLError as err:
            err_msg = f"Invalid YAML in run config {path}: {err}"
            raise ValueError(err_msg) from err
        except OSError as err:
            err_msg = f"Cannot read run config {path}: {err}"
            raise ValueError(err_msg) from err

        if not isinstance(config_dict, dict):
            err_msg = f"Run config {path} must be a mapping at the top level"
            raise TypeError(err_msg)

        return config_dict

    @staticmethod
    def _extract_mode_from_dot_config(
        config_dict: dict[str, Any],
        path: Path,
    ) -> str:
        """Determine the package mode from the contents of `config_dict`."""
        package = config_dict.get("package")
        if not isinstance(package, dict):
            err_msg = f"Running config {path} is missing the 'package' mapping."
            raise TypeError(err_msg)

        query_engine = package.get("query_engine")
        storage_engine = package.get("storage_engine")
        if query_engine is None or storage_engine is None:
            err_msg = (
                f"Running config {path} must specify both 'package.query_engine' and"
                " 'package.storage_engine'."
            )
            raise ValueError(err_msg)

        if query_engine == "clp" and storage_engine == "clp":
            return "clp-text"
        if query_engine == "clp-s" and storage_engine == "clp-s":
            return "clp-json"

        err_msg = (
            f"Run config {path} specifies running conditions for which integration testing is not"
            f"supported: query_engine={query_engine}, storage_engine={storage_engine}."
        )
        raise ValueError(err_msg)

    @staticmethod
    def _get_clp_instance_id(clp_instance_id_file_path: Path) -> str:
        """Return the 4-digit hexadecimal CLP instance-id stored at clp_instance_id_file_path."""
        try:
            contents = clp_instance_id_file_path.read_text(encoding="utf-8").strip()
        except OSError as err:
            err_msg = f"Cannot read instance-id file {clp_instance_id_file_path}: {err}"
            raise ValueError(err_msg) from err

        if not re.fullmatch(r"[0-9a-fA-F]{4}", contents):
            err_msg = (
                f"Invalid instance ID in {clp_instance_id_file_path}: expected a 4-character"
                f" hexadecimal string, but read {contents}."
            )
            raise ValueError(err_msg)

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
