import logging
import os
import pathlib
import shutil
import time
from datetime import datetime
from typing import List, Set

from bson import ObjectId
from clp_py_utils.clp_config import (
    ArchiveOutput,
    FsStorage,
    StorageEngine,
    StorageType,
)
from clp_py_utils.clp_logging import get_logging_formatter, set_logging_level
from clp_py_utils.s3_utils import s3_delete_objects
from job_orchestration.retention.constants import MIN_TO_SECONDS


def configure_logger(
    logger: logging.Logger, logging_level: str, log_directory: pathlib.Path, handler_name: str
):
    log_file = log_directory / f"{handler_name}.log"
    logging_file_handler = logging.FileHandler(filename=log_file, encoding="utf-8")
    logging_file_handler.setFormatter(get_logging_formatter())
    logger.addHandler(logging_file_handler)
    set_logging_level(logger, logging_level)


def validate_storage_type(output_config: ArchiveOutput, storage_engine: str) -> None:
    storage_type = output_config.storage.type
    if StorageType.S3 == storage_type:
        if StorageEngine.CLP_S != storage_engine:
            raise ValueError(
                f"{storage_type} is not supported when using storage engine {storage_engine}"
            )
    elif StorageType.FS != storage_type:
        raise ValueError(f"Unsupported Storage type: {storage_type}")


def get_expiry_epoch_secs(retention_minutes: int) -> int:
    """
    Returns a cutoff `expiry_epoch` based on the current timestamp and `retention_minutes`. Any
    target with a timestamp (`ts`) less than `expiry_epoch` are considered expired.
    The `expiry_epoch` is calculated as `expiry_epoch = cur_time - retention_secs`.

    :param: retention_minutes: Retention period in minutes.
    :return: The UTC epoch of the expiry time.
    """
    return int(time.time() - retention_minutes * MIN_TO_SECONDS)


def get_oid_with_expiry_time(expiry_epoch_secs: int) -> ObjectId:
    return ObjectId.from_datetime(datetime.utcfromtimestamp(expiry_epoch_secs))


def remove_fs_target(fs_storage_config: FsStorage, target: str) -> None:
    """
    Removes a target (either a directory or a file) from the filesystem storage. The full path
    of the target is constructed as `fs_storage_config.directory / target`. The function performs
    no action if the target does not exist.

    :param fs_storage_config:
    :param target: Relative path of the file or directory to remove.
    """
    path_to_remove = fs_storage_config.directory / target
    if not path_to_remove.exists():
        return

    if path_to_remove.is_dir():
        shutil.rmtree(path_to_remove)
    else:
        os.remove(path_to_remove)


def remove_targets(output_config: ArchiveOutput, targets: Set[str]) -> None:
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
    """
    A class representing an in-memory buffer for targets with fault-tolerance support.
    This class support recovering from a previous failure by reading previously persisted targets
    from a recovery file. The user is expected to explicitly call `persists_new_targets` to persist
    any new targets on to the disk for fault-tolerance purposes.

    :param recovery_file_path: Path to the file used for recovering and persisting targets.
    :raises ValueError: If the recovery path exists but is not a file.
    """

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
        """
        Writes any new targets added since initialization to the recovery file.
        """
        if len(self._targets_to_persist) == 0:
            return

        with open(self._recovery_file_path, "a") as f:
            for target in self._targets_to_persist:
                f.write(f"{target}\n")

        self._targets_to_persist.clear()

    def flush(self):
        """
        Clear the in-memory buffer of targets and remove the recovery file.
        This is intended to be called after the caller finished processing all targets (i.e. when
        recovery is no longer needed for the targets.)
        """
        self._targets.clear()
        if self._recovery_file_path.exists():
            os.unlink(self._recovery_file_path)
