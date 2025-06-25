import logging
import os
import pathlib
import shutil
import time
from datetime import datetime
from typing import List, Set, Union

from bson import ObjectId
from clp_py_utils.clp_config import (
    ArchiveOutput,
    FsStorage,
    StorageEngine,
    StorageType,
    StreamOutput,
)
from clp_py_utils.clp_logging import get_logging_formatter, set_logging_level
from clp_py_utils.s3_utils import s3_delete_objects
from job_orchestration.retention.constants import MIN_TO_SECONDS

MONGODB_ID_KEY = "_id"
MONGODB_STREAM_PATH_KEY = "path"
# TODO: Remove this constant and use the one defined in package once PR939 is merged.
RESULTS_METADATA_COLLECTION = "results-metadata"


def configure_logger(
    logger: logging.Logger, logging_level: str, log_directory: pathlib.Path, handler_name: str
):
    log_file = log_directory / f"{handler_name}.log"
    logging_file_handler = logging.FileHandler(filename=log_file, encoding="utf-8")
    logging_file_handler.setFormatter(get_logging_formatter())
    logger.addHandler(logging_file_handler)
    set_logging_level(logger, logging_level)


def validate_storage_type(
    output_config: Union[StreamOutput, ArchiveOutput], storage_engine: str
) -> None:
    storage_type = output_config.storage.type
    if StorageType.S3 == storage_type:
        if StorageEngine.CLP_S != storage_engine:
            raise ValueError(
                f"{storage_type} is not supported when using storage engine {storage_engine}"
            )
    elif StorageType.FS != storage_type:
        raise ValueError(f"Unsupported Storage type: {storage_type}")


def get_expiry_epoch_secs(retention_minutes: int) -> int:
    return int(time.time() - retention_minutes * MIN_TO_SECONDS)


def remove_fs_target(fs_storage_config: FsStorage, relative_path: str) -> None:
    path_to_remove = fs_storage_config.directory / relative_path
    if not path_to_remove.exists():
        return

    if path_to_remove.is_dir():
        shutil.rmtree(path_to_remove)
    else:
        os.remove(path_to_remove)


def get_oid_with_expiry_time(expiry_epoch_secs: int) -> ObjectId:
    return ObjectId.from_datetime(datetime.utcfromtimestamp(expiry_epoch_secs))


def remove_targets(output_config: Union[ArchiveOutput, StreamOutput], targets: Set[str]) -> None:
    storage_config = output_config.storage
    storage_type = storage_config.type

    if StorageType.S3 == storage_type:
        s3_delete_objects(storage_config.s3_config, targets)
    elif StorageType.FS == storage_type:
        for target in targets:
            remove_fs_target(storage_config, target)
    else:
        raise ValueError(f"Unsupported Storage type: {storage_type}")


class TargetsBuffer:
    def __init__(self, recovery_file_path: pathlib.Path):
        self._recovery_file_path: pathlib.Path = recovery_file_path
        self._targets: Set[str] = set()
        self._targets_to_persist: List[str] = list()

        if not self._recovery_file_path.exists():
            return

        if not self._recovery_file_path.is_file():
            raise ValueError(f"{self._recovery_file_path} does not resolve to a file")

        with open(self._recovery_file_path, "r") as f:
            for line in f:
                self._targets.add(line.strip())

    def add_target(self, target: str) -> None:
        if target not in self._targets:
            self._targets.add(target)
            self._targets_to_persist.append(target)

    def get_targets(self) -> Set[str]:
        return self._targets

    def persists_new_targets(self) -> None:
        if len(self._targets_to_persist) == 0:
            return

        with open(self._recovery_file_path, "a") as f:
            for target in self._targets_to_persist:
                f.write(f"{target}\n")

        self._targets_to_persist.clear()

    def flush(self):
        self._targets.clear()
        if self._recovery_file_path.exists():
            os.unlink(self._recovery_file_path)
