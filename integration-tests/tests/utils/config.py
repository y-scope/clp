"""Define all python classes used in `integration-tests`."""

import re
from dataclasses import dataclass, field, InitVar
from pathlib import Path

from clp_py_utils.clp_config import (
    QueryEngine,
    StorageEngine,
)

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
class PackageModeConfig:
    """Defines details related to a package instance's mode of operation."""

    name: str
    storage_engine: StorageEngine
    query_engine: QueryEngine


@dataclass(frozen=True)
class PackageInstanceConfig:
    """Metadata for the clp-config.yml file used to configure a clp package instance."""

    #: The PackageConfig object corresponding to this package run.
    package_config: PackageConfig

    #: The PackageModeConfig object describing this config file's configuration.
    mode_config: PackageModeConfig

    #: The location of the configfile used during this package run.
    temp_config_file_path: Path = field(init=False, repr=True)


@dataclass(frozen=True)
class PackageInstance:
    """Metadata for a run of the clp package."""

    #:
    package_instance_config: PackageInstanceConfig

    #:
    clp_log_dir: Path = field(init=False, repr=True)

    #:
    clp_instance_id: str = field(init=False, repr=True)

    #: The path to the .clp-config.yml file constructed by the package during spin-up.
    shared_config_file_path: Path = field(init=False, repr=True)

    def __post_init__(self) -> None:
        """Validates the values specified at init, and initialises attributes."""
        # Set clp_log_dir and validate that it exists.
        clp_log_dir = self.package_instance_config.package_config.clp_package_dir / "var" / "log"
        validate_dir_exists(clp_log_dir)
        object.__setattr__(self, "clp_log_dir", clp_log_dir)

        # Set clp_instance_id.
        clp_instance_id_file_path = self.clp_log_dir / "instance-id"
        validate_file_exists(clp_instance_id_file_path)
        clp_instance_id = self._get_clp_instance_id(clp_instance_id_file_path)
        object.__setattr__(self, "clp_instance_id", clp_instance_id)

        # Set shared_config_file_path after validating it.
        shared_config_file_path = self.clp_log_dir / ".clp-config.yml"
        validate_file_exists(shared_config_file_path)
        object.__setattr__(self, "shared_config_file_path", shared_config_file_path)

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
