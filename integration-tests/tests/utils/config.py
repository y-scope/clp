import shutil
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class PackageConfig:
    clp_bin_dir: Path
    clp_package_dir: Path
    clp_sbin_dir: Path
    test_output_dir: Path
    uncompressed_logs_dir: Path


@dataclass(frozen=True)
class DatasetLogs:
    dataset_name: str
    tarball_url: str
    download_dir: Path
    extraction_dir: Path
    compression_dir: Path
    decompression_dir: Path

    @classmethod
    def create(cls, package_config, dataset_name: str, **kwargs):
        dataset_name = dataset_name.strip()
        if 0 == len(dataset_name):
            raise ValueError("`dataset_name` cannot be empty.")
        return cls(
            dataset_name=dataset_name,
            tarball_url=kwargs.get("tarball_url", ""),
            download_dir=kwargs.get(
                "download_dir", package_config.uncompressed_logs_dir / f"{dataset_name}.tar.gz"
            ),
            extraction_dir=kwargs.get(
                "extraction_dir", package_config.uncompressed_logs_dir / dataset_name
            ),
            compression_dir=kwargs.get(
                "compression_dir", package_config.test_output_dir / f"{dataset_name}-archives"
            ),
            decompression_dir=kwargs.get(
                "decompression_dir",
                package_config.test_output_dir / f"{dataset_name}-decompressed-logs",
            ),
        )

    def clear_test_outputs(self) -> None:
        """Remove compression and decompression directories for test init and cleanup."""
        shutil.rmtree(self.compression_dir, ignore_errors=True)
        shutil.rmtree(self.decompression_dir, ignore_errors=True)
