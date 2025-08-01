import shutil
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class PackageConfig:
    clp_bin_dir: Path
    clp_package_dir: Path
    clp_sbin_dir: Path
    test_root_dir: Path
    uncompressed_logs_dir: Path


@dataclass(frozen=True)
class DatasetLogs:
    dataset_name: str
    tarball_url: str
    tarball_path: Path
    extraction_dir: Path

    @classmethod
    def create(cls, package_config: PackageConfig, dataset_name: str, tarball_url: str):
        dataset_name = dataset_name.strip()
        if 0 == len(dataset_name):
            raise ValueError("`dataset_name` cannot be empty.")
        return cls(
            dataset_name=dataset_name,
            tarball_url=tarball_url,
            tarball_path=package_config.uncompressed_logs_dir / f"{dataset_name}.tar.gz",
            extraction_dir=package_config.uncompressed_logs_dir / dataset_name,
        )


@dataclass(frozen=True)
class CompressionTestPaths:
    test_name: str
    logs_source_dir: Path
    compression_dir: Path
    decompression_dir: Path

    @classmethod
    def create(cls, package_config: PackageConfig, test_name: str, logs_source_dir: Path):
        return cls(
            test_name=test_name,
            logs_source_dir=logs_source_dir,
            compression_dir=package_config.test_root_dir / f"{test_name}-archives",
            decompression_dir=package_config.test_root_dir / f"{test_name}-decompressed-logs",
        )

    def clear_test_outputs(self) -> None:
        shutil.rmtree(self.compression_dir, ignore_errors=True)
        shutil.rmtree(self.decompression_dir, ignore_errors=True)
