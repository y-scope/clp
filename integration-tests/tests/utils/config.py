import shutil
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class TestConfig:
    __test__ = False

    clp_package_dir: Path
    logs_source_dir: Path
    test_root_dir: Path

    def get_clp_binary_path(self) -> Path:
        return self.clp_package_dir / "bin" / "clp"

    def get_clp_s_binary_path(self) -> Path:
        return self.clp_package_dir / "bin" / "clp-s"


@dataclass(frozen=True)
class TestLogs:
    __test__ = False

    name: str
    tarball_url: str
    tarball_path: Path
    extraction_dir: Path

    @classmethod
    def create(cls, name: str, tarball_url: str, test_config: TestConfig):
        name = name.strip()
        if 0 == len(name):
            raise ValueError("`name` cannot be empty.")
        return cls(
            name=name,
            tarball_url=tarball_url,
            tarball_path=test_config.logs_source_dir / f"{name}.tar.gz",
            extraction_dir=test_config.logs_source_dir / name,
        )


@dataclass(frozen=True)
class CompressionTestConfig:
    test_name: str
    logs_source_dir: Path
    compression_dir: Path
    decompression_dir: Path

    @classmethod
    def create(cls, test_name: str, logs_source_dir: Path, test_config: TestConfig):
        return cls(
            test_name=test_name,
            logs_source_dir=logs_source_dir,
            compression_dir=test_config.test_root_dir / f"{test_name}-archives",
            decompression_dir=test_config.test_root_dir / f"{test_name}-decompressed-logs",
        )

    def clear_test_outputs(self) -> None:
        if self.compression_dir.exists():
            shutil.rmtree(self.compression_dir)
        if self.decompression_dir.exists():
            shutil.rmtree(self.decompression_dir)
