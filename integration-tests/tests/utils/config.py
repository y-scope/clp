"""Define all python classes used in `integration-tests`."""

from __future__ import annotations

import re
from dataclasses import dataclass, field, InitVar
from pathlib import Path

import yaml
from clp_py_utils.clp_config import (
    CLP_DEFAULT_LOG_DIRECTORY_PATH,
    ClpConfig,
)

from tests.utils.utils import (
    unlink,
    validate_dir_exists,
    validate_file_exists,
)


@dataclass(frozen=True)
class ClpCorePathConfig:
    """Path configuration for the CLP core binaries."""

    #: Root directory containing all CLP core binaries.
    clp_core_bins_dir: Path

    def __post_init__(self) -> None:
        """
        Validates that the CLP core binaries directory exists and contains all required
        executables.
        """
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
                f"CLP core binaries at {clp_core_bins_dir} are incomplete."
                f" Missing binaries: {', '.join(missing_binaries)}"
            )
            raise RuntimeError(err_msg)

    @property
    def clp_binary_path(self) -> Path:
        """:return: The absolute path to the core binary `clp`."""
        return self.clp_core_bins_dir / "clp"

    @property
    def clp_s_binary_path(self) -> Path:
        """:return: The absolute path to the core binary `clp-s`."""
        return self.clp_core_bins_dir / "clp-s"


@dataclass(frozen=True)
class PackagePathConfig:
    """Path configuration for the CLP package."""

    #: Root directory containing all CLP package contents.
    clp_package_dir: Path

    #: Root directory for package tests output.
    test_root_dir: InitVar[Path]

    #: Directory to store temporary package config files.
    temp_config_dir: Path = field(init=False, repr=True)

    #: Directory where the CLP package writes logs.
    clp_log_dir: Path = field(init=False, repr=True)

    def __post_init__(self, test_root_dir: Path) -> None:
        """Validates init values and initializes attributes."""
        # Validate that the CLP package directory exists and contains required directories.
        clp_package_dir = self.clp_package_dir
        validate_dir_exists(clp_package_dir)

        required_dirs = ["etc", "sbin"]
        missing_dirs = [d for d in required_dirs if not (clp_package_dir / d).is_dir()]
        if len(missing_dirs) > 0:
            err_msg = (
                f"CLP package at {clp_package_dir} is incomplete."
                f" Missing directories: {', '.join(missing_dirs)}"
            )
            raise RuntimeError(err_msg)

        # Initialize directory for package tests.
        validate_dir_exists(test_root_dir)
        object.__setattr__(self, "temp_config_dir", test_root_dir / "temp_config_files")

        # Initialize log directory for the package.
        object.__setattr__(
            self,
            "clp_log_dir",
            clp_package_dir / CLP_DEFAULT_LOG_DIRECTORY_PATH,
        )

        # Create directories if they do not already exist.
        self.temp_config_dir.mkdir(parents=True, exist_ok=True)
        self.clp_log_dir.mkdir(parents=True, exist_ok=True)

    @property
    def start_script_path(self) -> Path:
        """:return: The absolute path to the package start script."""
        return self.clp_package_dir / "sbin" / "start-clp.sh"

    @property
    def stop_script_path(self) -> Path:
        """:return: The absolute path to the package stop script."""
        return self.clp_package_dir / "sbin" / "stop-clp.sh"


@dataclass(frozen=True)
class PackageConfig:
    """Metadata for a specific configuration of the CLP package."""

    #: Path configuration for this package.
    path_config: PackagePathConfig

    #: Name of the package operation mode.
    mode_name: str

    #: The list of CLP components that this package needs.
    component_list: list[str]

    #: The Pydantic representation of a CLP package configuration.
    clp_config: ClpConfig

    #: The base port from which all ports for the components are derived.
    base_port: int

    def __post_init__(self) -> None:
        """Write the temporary config file for this package."""
        self._write_temp_config_file()

    @property
    def temp_config_file_path(self) -> Path:
        """:return: The absolute path to the temporary configuration file for the package."""
        return self.path_config.temp_config_dir / f"clp-config-{self.mode_name}.yaml"

    def _write_temp_config_file(self) -> None:
        """Writes the temporary config file for this package."""
        temp_config_file_path = self.temp_config_file_path

        payload = self.clp_config.dump_to_primitive_dict()  # type: ignore[no-untyped-call]

        tmp_path = temp_config_file_path.with_suffix(temp_config_file_path.suffix + ".tmp")
        with tmp_path.open("w", encoding="utf-8") as f:
            yaml.safe_dump(payload, f, sort_keys=False)
        tmp_path.replace(temp_config_file_path)


@dataclass(frozen=True)
class PackageInstance:
    """Metadata for a running instance of the CLP package."""

    #: The configuration for this package instance.
    package_config: PackageConfig

    #: The instance ID of the running package.
    clp_instance_id: str = field(init=False, repr=True)

    def __post_init__(self) -> None:
        """Validates init values and initializes attributes."""
        # Validate that the temp config file exists.
        validate_file_exists(self.package_config.temp_config_file_path)

        # Set clp_instance_id from instance-id file.
        path_config = self.package_config.path_config
        clp_instance_id_file_path = path_config.clp_log_dir / "instance-id"
        validate_file_exists(clp_instance_id_file_path)
        clp_instance_id = self._get_clp_instance_id(clp_instance_id_file_path)
        object.__setattr__(self, "clp_instance_id", clp_instance_id)

    @staticmethod
    def _get_clp_instance_id(clp_instance_id_file_path: Path) -> str:
        """
        Reads the CLP instance ID from the given file and validates its format.

        :param clp_instance_id_file_path:
        :return: The 4-character hexadecimal instance ID.
        :raise ValueError: If the file cannot be read or contents are not a 4-character hex string.
        """
        try:
            contents = clp_instance_id_file_path.read_text(encoding="utf-8").strip()
        except OSError as err:
            err_msg = f"Cannot read instance-id file '{clp_instance_id_file_path}'"
            raise ValueError(err_msg) from err

        if not re.fullmatch(r"[0-9a-fA-F]{4}", contents):
            err_msg = (
                f"Invalid instance ID in {clp_instance_id_file_path}: expected a 4-character"
                f" hexadecimal string, but read {contents}."
            )
            raise ValueError(err_msg)

        return contents


@dataclass(frozen=True)
class IntegrationTestPathConfig:
    """Path configuration for CLP integration tests."""

    #: Default directory for integration test output.
    test_root_dir: Path

    #: Directory to store the downloaded logs.
    logs_download_dir: Path = field(init=False, repr=True)

    #: Optional initialization value used to set `logs_download_dir`.
    logs_download_dir_init: InitVar[Path | None] = None

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
    integration_test_path_config: InitVar[IntegrationTestPathConfig]
    #:
    tarball_path: Path = field(init=False, repr=True)
    #:
    extraction_dir: Path = field(init=False, repr=True)

    def __post_init__(self, integration_test_path_config: IntegrationTestPathConfig) -> None:
        """Initialize and set tarball and extraction paths for integration test logs."""
        name = self.name.strip()
        if 0 == len(name):
            err_msg = "`name` cannot be empty."
            raise ValueError(err_msg)
        logs_download_dir = integration_test_path_config.logs_download_dir
        validate_dir_exists(logs_download_dir)

        object.__setattr__(self, "name", name)
        object.__setattr__(self, "tarball_path", logs_download_dir / f"{name}.tar.gz")
        object.__setattr__(self, "extraction_dir", logs_download_dir / name)


@dataclass(frozen=True)
class CompressionTestPathConfig:
    """Per-test path configuration for compression workflow artifacts."""

    #:
    test_name: str
    #: Directory containing the original (uncompressed) log files used by this test.
    logs_source_dir: Path
    integration_test_path_config: InitVar[IntegrationTestPathConfig]
    #: Path to store compressed archives generated by the test.
    compression_dir: Path = field(init=False, repr=True)
    #: Path to store decompressed logs generated by the test.
    decompression_dir: Path = field(init=False, repr=True)

    def __post_init__(self, integration_test_path_config: IntegrationTestPathConfig) -> None:
        """Initialize and set required directory paths for compression tests."""
        test_name = self.test_name.strip()
        if 0 == len(test_name):
            err_msg = "`test_name` cannot be empty."
            raise ValueError(err_msg)
        test_root_dir = integration_test_path_config.test_root_dir
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
