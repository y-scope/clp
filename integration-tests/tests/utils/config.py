import shutil
from dataclasses import dataclass, field, InitVar
from pathlib import Path


@dataclass(frozen=True)
class IntegrationTestConfig:
    clp_package_dir: Path
    test_root_dir: Path
    logs_source_dir: Path = field(default=Path("/"))

    def __post_init__(self):
        if Path("/") == self.logs_source_dir:
            object.__setattr__(self, "logs_source_dir", self.test_root_dir / "downloads")

        # Check for required directories
        required_dirs = ["bin", "etc", "lib", "sbin"]
        missing_dirs = [d for d in required_dirs if not (self.clp_package_dir / d).is_dir()]
        if len(missing_dirs) > 0:
            raise ValueError(
                f"CLP package at {self.clp_package_dir} is incomplete. "
                f"Missing: {', '.join(missing_dirs)}"
            )

        self.test_root_dir.mkdir(parents=True, exist_ok=True)
        self.logs_source_dir.mkdir(parents=True, exist_ok=True)

    def get_clp_binary_path(self) -> Path:
        return self.clp_package_dir / "bin" / "clp"

    def get_clp_s_binary_path(self) -> Path:
        return self.clp_package_dir / "bin" / "clp-s"


@dataclass(frozen=True)
class IntegrationTestLogs:
    name: str
    tarball_url: str
    integration_test_config: InitVar[IntegrationTestConfig]
    tarball_path: Path = field(init=False, repr=True)
    extraction_dir: Path = field(init=False, repr=True)

    def __post_init__(self, integration_test_config: IntegrationTestConfig):
        name = self.name.strip()
        if 0 == len(name):
            raise ValueError("`name` cannot be empty.")

        object.__setattr__(self, "name", name)
        object.__setattr__(
            self, "tarball_path", integration_test_config.logs_source_dir / f"{name}.tar.gz"
        )
        object.__setattr__(self, "extraction_dir", integration_test_config.logs_source_dir / name)


@dataclass(frozen=True)
class CompressionTestConfig:
    test_name: str
    logs_source_dir: Path
    integration_test_config: InitVar[IntegrationTestConfig]
    compression_dir: Path = field(init=False, repr=True)
    decompression_dir: Path = field(init=False, repr=True)

    def __post_init__(self, integration_test_config: IntegrationTestConfig):
        test_name = self.test_name.strip()
        if 0 == len(test_name):
            raise ValueError("`test_name` cannot be empty.")

        object.__setattr__(self, "test_name", test_name)
        object.__setattr__(
            self, "compression_dir", integration_test_config.test_root_dir / f"{test_name}-archives"
        )
        object.__setattr__(
            self,
            "decompression_dir",
            integration_test_config.test_root_dir / f"{test_name}-decompressed-logs",
        )

    def clear_test_outputs(self) -> None:
        if self.compression_dir.exists():
            shutil.rmtree(self.compression_dir)
        if self.decompression_dir.exists():
            shutil.rmtree(self.decompression_dir)
