from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class PackageTestConfig:
    clp_bin_dir: Path
    clp_package_dir: Path
    clp_sbin_dir: Path
    test_output_dir: Path
    uncompressed_logs_dir: Path


@dataclass(frozen=True)
class DatasetParams:
    dataset_name: str
    tar_url: str
