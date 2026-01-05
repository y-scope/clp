import logging
import os
import pathlib
import shutil
import time
from datetime import datetime, timezone

from bson import ObjectId
from clp_py_utils.clp_config import (
    ArchiveOutput,
    FsStorage,
    StorageEngine,
    StorageType,
)
from clp_py_utils.clp_logging import get_logging_formatter, set_logging_level
from clp_py_utils.s3_utils import s3_delete_objects

from job_orchestration.garbage_collector.constants import MIN_TO_SECONDS


def configure_logger(logger: logging.Logger, logging_level: str):
    """Configure the logging level for a logger (logs go to stdout, captured by Docker)."""
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
    candidate with a timestamp (`ts`) less than `expiry_epoch` is considered expired.
    The `expiry_epoch` is calculated as `expiry_epoch = cur_time - retention_secs`.

    :param retention_minutes: Retention period in minutes.
    :return: The UTC epoch representing the expiry cutoff time.
    """
    return int(time.time() - retention_minutes * MIN_TO_SECONDS)


def get_oid_with_expiry_time(expiry_epoch_secs: int) -> ObjectId:
    return ObjectId.from_datetime(datetime.fromtimestamp(expiry_epoch_secs, tz=timezone.utc))


def execute_fs_deletion(fs_storage_config: FsStorage, candidate: str) -> None:
    """
    Deletes a candidate (either a directory or a file) from the filesystem storage. The full path
    of the candidate is constructed as `fs_storage_config.directory / candidate`. The function
    performs no action if the candidate does not exist.

    :param fs_storage_config:
    :param candidate: Relative path of the file or directory to delete.
    """
    path_to_delete = fs_storage_config.directory / candidate
    if not path_to_delete.exists():
        return

    if path_to_delete.is_dir():
        shutil.rmtree(path_to_delete)
    else:
        os.remove(path_to_delete)


def execute_deletion(output_config: ArchiveOutput, deletion_candidates: set[str]) -> None:
    storage_config = output_config.storage
    storage_type = storage_config.type

    if StorageType.S3 == storage_type:
        s3_delete_objects(storage_config.s3_config, deletion_candidates)
    elif StorageType.FS == storage_type:
        for candidate in deletion_candidates:
            execute_fs_deletion(storage_config, candidate)
    else:
        raise ValueError(f"Unsupported Storage type: {storage_type}")


class DeletionCandidatesBuffer:
    """
    Represents an in-memory buffer for deletion candidates with fault-tolerance support.

    This class supports recovering from a previous failure by reading previously persisted
    candidates from a recovery file. The user is expected to explicitly call
    `persist_new_candidates` to persist any new candidates on to the disk for fault-tolerance
    purposes.

    :param recovery_file_path: Path to the file used for recovering and persisting candidates.
    :raises ValueError: If the recovery path exists but is not a file.
    """

    def __init__(self, recovery_file_path: pathlib.Path):
        self._recovery_file_path: pathlib.Path = recovery_file_path
        self._candidates: set[str] = set()
        self._candidates_to_persist: list[str] = list()

        if not self._recovery_file_path.exists():
            return

        if not self._recovery_file_path.is_file():
            raise ValueError(f"{self._recovery_file_path} does not resolve to a file")

        with open(self._recovery_file_path, "r") as f:
            for line in f:
                self._candidates.add(line.strip())

    def add_candidate(self, candidate: str) -> None:
        if candidate not in self._candidates:
            self._candidates.add(candidate)
            self._candidates_to_persist.append(candidate)

    def get_candidates(self) -> set[str]:
        return self._candidates

    def persist_new_candidates(self) -> None:
        """
        Persists any new deletion candidates buffered in `_candidates_to_persist` by appending them
        to the recovery file, then clears `_candidates_to_persist`.
        This method returns immediately if `_candidates_to_persist` is empty.
        """
        if len(self._candidates_to_persist) == 0:
            return

        with open(self._recovery_file_path, "a") as f:
            f.writelines(f"{candidate}\n" for candidate in self._candidates_to_persist)

        self._candidates_to_persist.clear()

    def clear(self):
        """
        Clears the in-memory buffer of candidates and removes the recovery file.

        This is intended to be called after the caller finished processing all candidates (i.e.,
        when recovery is no longer needed for the candidates.)
        """
        self._candidates.clear()
        if self._recovery_file_path.exists():
            os.unlink(self._recovery_file_path)
