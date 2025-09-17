"""Define all python classes used in `integration-tests`."""

from __future__ import annotations

from dataclasses import dataclass, field, InitVar
from pathlib import Path

from tests.utils.utils import (
    unlink,
    validate_dir_exists_and_is_absolute,
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
        validate_dir_exists_and_is_absolute(clp_core_bins_dir)

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
class DepsConfig:
    """The configuration for dependencies used by clp."""

    #:
    clp_deps_core_dir: Path

    def __post_init__(self) -> None:
        """Validates that the core dependency directory exists."""
        validate_dir_exists_and_is_absolute(self.clp_deps_core_dir)

    @property
    def lz4_binary_path(self) -> Path:
        """:return: The absolute path to the lz4 compression tool."""
        return self.clp_deps_core_dir / "lz4-install" / "bin" / "lz4"

    @property
    def zstd_binary_path(self) -> Path:
        """:return: The absolute path to the zstd compression tool."""
        return self.clp_deps_core_dir / "zstd-install" / "bin" / "zstd"

    @property
    def xz_binary_path(self) -> Path:
        """:return: The absolute path to the LibLZMA xz compression tool."""
        return self.clp_deps_core_dir / "LibLZMA-static-install" / "bin" / "xz"



@dataclass(frozen=True)
class PackageConfig:
    """The configuration for the clp package subject to testing."""

    #:
    clp_package_dir: Path

    def __post_init__(self) -> None:
        """Validates that the CLP package directory exists and contains all required directories."""
        clp_package_dir = self.clp_package_dir
        validate_dir_exists_and_is_absolute(clp_package_dir)

        # Check for required package script directories
        required_dirs = ["bin", "etc", "lib", "sbin"]
        missing_dirs = [d for d in required_dirs if not (clp_package_dir / d).is_dir()]
        if len(missing_dirs) > 0:
            err_msg = (
                f"CLP package at {clp_package_dir} is incomplete."
                f" Missing directories: {', '.join(missing_dirs)}"
            )
            raise ValueError(err_msg)


@dataclass(frozen=True)
class IntegrationTestConfig:
    """The general configuration for integration tests."""

    #:
    core_config: CoreConfig
    #:
    deps_config: DepsConfig
    #:
    package_config: PackageConfig
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
    extraction_dir: Path = field(init=False, repr=True)

    def __post_init__(self, integration_test_config: IntegrationTestConfig) -> None:
        """Initialize and set tarball and extraction paths for integration test logs."""
        name = self.name.strip()
        if 0 == len(name):
            err_msg = "`name` cannot be empty."
            raise ValueError(err_msg)
        logs_download_dir = integration_test_config.logs_download_dir
        validate_dir_exists_and_is_absolute(logs_download_dir)

        object.__setattr__(self, "name", name)
        object.__setattr__(self, "extraction_dir", logs_download_dir / name)

    @property
    def base_tar_path(self) -> None:
        return self.extraction_dir.with_suffix(".tar")

    @property
    def tar_gz_path(self) -> None:
        return self.extraction_dir.with_suffix(".tar.gz")

    @property
    def tar_lz4_path(self) -> None:
        return self.extraction_dir.with_suffix(".tar.lz4")

    @property
    def tar_xz_path(self) -> None:
        return self.extraction_dir.with_suffix(".tar.xz")

    @property
    def tar_zstd_path(self) -> None:
        return self.extraction_dir.with_suffix(".tar.zstd")



@dataclass(frozen=True)
class CompressionTestConfig:
    """Compression test configuration providing per-test metadata for artifacts and directories."""

    #:
    test_name: str
    #: Path to the CLP compressionm input archive or directory.
    compression_input: Path
    integration_test_config: InitVar[IntegrationTestConfig]
    #: Directory to store generated compressed CLP archives.
    compression_dir: Path = field(init=False, repr=True)
    #: Directory to store logs decompressed from CLP archives.
    decompression_dir: Path = field(init=False, repr=True)

    def __post_init__(self, integration_test_config: IntegrationTestConfig) -> None:
        """Initialize and set required directory paths for compression tests."""
        test_name = self.test_name.strip()
        if 0 == len(test_name):
            err_msg = "`test_name` cannot be empty."
            raise ValueError(err_msg)
        test_root_dir = integration_test_config.test_root_dir
        validate_dir_exists_and_is_absolute(test_root_dir)

        object.__setattr__(self, "test_name", test_name)
        object.__setattr__(self, "compression_dir", test_root_dir / f"{test_name}-archives")
        object.__setattr__(
            self, "decompression_dir", test_root_dir / f"{test_name}-decompressed-logs"
        )

    def clear_test_outputs(self) -> None:
        """Remove any existing output directories created by this compression test."""
        unlink(self.compression_dir)
        unlink(self.decompression_dir)
