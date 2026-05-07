"""Classes used in CLP package integration tests."""

import logging
import re
from dataclasses import dataclass
from pathlib import Path

import pytest
from clp_py_utils.clp_config import (
    ClpConfig,
)
from pydantic import ValidationError

from tests.utils.classes import IntegrationTestPathConfig
from tests.utils.utils import (
    load_yaml_to_dict,
    validate_dir_exists,
)

logger = logging.getLogger(__name__)


@dataclass
class ClpPackageTestPathConfig(IntegrationTestPathConfig):
    """Path configuration for CLP package integration tests."""

    #: Default CLP package directory.
    clp_package_dir: Path

    def __post_init__(self) -> None:
        """Create and validate directories."""
        # Validate that init directory exists.
        validate_dir_exists(self.clp_package_dir)

        super().__post_init__()

        # Create `temp_config_dir`.
        self.temp_config_dir.mkdir(parents=True, exist_ok=True)

        # Create `package_decompression_dir`.
        self.package_decompression_dir.mkdir(parents=True, exist_ok=True)

    @property
    def archive_manager_path(self) -> Path:
        """:return: The absolute path to the package archive-manager script."""
        return self.clp_package_dir / "sbin" / "admin-tools" / "archive-manager.sh"

    @property
    def compress_from_s3_path(self) -> Path:
        """:return: The absolute path to the package compress-from-s3 script."""
        return self.clp_package_dir / "sbin" / "compress-from-s3.sh"

    @property
    def compress_path(self) -> Path:
        """:return: The absolute path to the package compress script."""
        return self.clp_package_dir / "sbin" / "compress.sh"

    @property
    def dataset_manager_path(self) -> Path:
        """:return: The absolute path to the package dataset-manager script."""
        return self.clp_package_dir / "sbin" / "admin-tools" / "dataset-manager.sh"

    @property
    def decompress_path(self) -> Path:
        """:return: The absolute path to the package decompress script."""
        return self.clp_package_dir / "sbin" / "decompress.sh"

    @property
    def package_archives_path(self) -> Path:
        """:return: The absolute path to the package archives."""
        return self.clp_package_dir / "var" / "data" / "archives"

    @property
    def package_decompression_dir(self) -> Path:
        """:return: The absolute path to the directory storing temporary decompressed files."""
        return self.test_cache_dir / "package_decompression"

    @property
    def package_logs_path(self) -> Path:
        """:return: The absolute path to the package logs."""
        return self.clp_package_dir / "var" / "log"

    @property
    def search_path(self) -> Path:
        """:return: The absolute path to the package search script."""
        return self.clp_package_dir / "sbin" / "search.sh"

    @property
    def start_clp_path(self) -> Path:
        """:return: The absolute path to the package start-clp script."""
        return self.clp_package_dir / "sbin" / "start-clp.sh"

    @property
    def stop_clp_path(self) -> Path:
        """:return: The absolute path to the package stop-clp script."""
        return self.clp_package_dir / "sbin" / "stop-clp.sh"

    @property
    def temp_config_dir(self) -> Path:
        """:return: The absolute path to the directory storing temporary package config files."""
        return self.test_cache_dir / "temp_configs"

    def _static_paths(self) -> list[Path]:
        """:return: Paths that must exist on disk at construction time."""
        return [
            *super()._static_paths(),
            self.archive_manager_path,
            self.compress_from_s3_path,
            self.compress_path,
            self.dataset_manager_path,
            self.decompress_path,
            self.search_path,
            self.start_clp_path,
            self.stop_clp_path,
        ]


@dataclass
class ClpPackageModeConfig:
    """Mode configuration for the CLP package."""

    #: Name of the package operation mode.
    mode_name: str

    #: The Pydantic representation of the package operation mode.
    clp_config: ClpConfig

    #: The list of CLP components that this package needs.
    component_list: tuple[str, ...]


@dataclass
class ClpPackage:
    """Metadata for the CLP package."""

    # The `ClpPackageTestPathConfig` object for this CLP package.
    path_config: ClpPackageTestPathConfig

    #: Name of the CLP package operating mode.
    mode_name: str

    #: The Pydantic representation of this CLP package's operation mode.
    clp_config: ClpConfig

    #: The list of CLP components that this CLP package needs.
    component_list: tuple[str, ...]

    def __post_init__(self) -> None:
        """Validate data members."""
        # Validate the `clp_config` pydantic object. This object is not validated elsewhere.
        log_msg = f"Validating the ClpConfig pydantic object for the '{self.mode_name}' package."
        logger.info(log_msg)
        try:
            ClpConfig.model_validate(self.clp_config)
        except ValidationError as err:
            fail_msg = (
                f"The `ClpConfig` pydantic object for the CLP package could not be validated: {err}"
            )
            pytest.fail(fail_msg)

    @property
    def temp_config_file_path(self) -> Path:
        """:return: The absolute path to the temporary configuration file for the package."""
        return self.path_config.temp_config_dir / f"clp-config-{self.mode_name}.yaml"

    @property
    def clp_instance_id_file_path(self) -> Path:
        """:return: The absolute path to the package instance-id file."""
        return self.path_config.package_logs_path / "instance-id"

    @property
    def shared_config_file_path(self) -> Path:
        """:return: The absolute path to the package shared config file."""
        return self.path_config.package_logs_path / ".clp-config.yaml"

    def get_clp_instance_id(self) -> str:
        """
        Reads the CLP instance ID for the package and validates its format.

        :return: The instance ID (4-character hex, 32-character hex, or full UUID).
        :raise ValueError: If the file cannot be read or contents are not a valid instance ID.
        """
        clp_instance_id_file_path = self.clp_instance_id_file_path
        try:
            contents = clp_instance_id_file_path.read_text(encoding="utf-8").strip()
        except OSError as err:
            err_msg = f"Cannot read instance-id file '{clp_instance_id_file_path}': {err}"
            pytest.fail(err_msg)

        uuid_pattern = (
            r"[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}"
        )
        if not re.fullmatch(r"[0-9a-fA-F]{4}|[0-9a-fA-F]{32}|" + uuid_pattern, contents):
            err_msg = (
                f"Invalid instance ID in {clp_instance_id_file_path}: expected a 4-character"
                f" hex string, a 32-character hex string, or a full UUID, but read {contents}."
            )
            pytest.fail(err_msg)

        return contents

    def get_running_config_from_shared_config_file(self) -> ClpConfig:
        """
        Loads the content of the shared config file into a ClpConfig pydantic object.

        :return: The ClpConfig object.
        """
        shared_config_dict = load_yaml_to_dict(self.shared_config_file_path)
        try:
            running_config = ClpConfig.model_validate(shared_config_dict)
        except ValidationError as err:
            fail_msg = f"The shared config file could not be validated: {err}"
            pytest.fail(fail_msg)

        return running_config
