import asyncio
import pathlib
import time
import uuid
from collections import defaultdict
from contextlib import closing

from clp_py_utils.clp_config import (
    ArchiveOutput,
    ClpConfig,
    Database,
    QUERY_JOBS_TABLE_NAME,
    StorageEngine,
)
from clp_py_utils.clp_logging import configure_logging, get_logger
from clp_py_utils.clp_metadata_db_utils import (
    delete_archives_from_metadata_db,
    get_archives_table_name,
    get_datasets_table_name,
)
from clp_py_utils.sql_adapter import SqlAdapter

from job_orchestration.garbage_collector.constants import (
    ARCHIVE_GARBAGE_COLLECTOR_NAME,
    MIN_TO_SECONDS,
    SECOND_TO_MILLISECOND,
)
from job_orchestration.garbage_collector.utils import (
    DeletionCandidatesBuffer,
    execute_deletion,
    validate_storage_type,
)
from job_orchestration.scheduler.constants import QueryJobStatus

logger = get_logger(ARCHIVE_GARBAGE_COLLECTOR_NAME)


def _delete_expired_archives(
    db_conn,
    db_cursor,
    table_prefix: str,
    expiry_base_epoch_secs: int,
    candidates_buffer: DeletionCandidatesBuffer,
    archive_output_config: ArchiveOutput,
) -> None:
    archives_table = get_archives_table_name(table_prefix)
    datasets_table = get_datasets_table_name(table_prefix)

    db_cursor.execute(
        f"""
        SELECT archives.id, archives.uuid, archives.dataset_id, datasets.name AS dataset
        FROM `{archives_table}` AS archives
        JOIN `{datasets_table}` AS datasets ON archives.dataset_id = datasets.id
        WHERE datasets.retention_period_minutes IS NOT NULL
        AND archives.creation_time_millis
            < (%s - datasets.retention_period_minutes * {MIN_TO_SECONDS})
              * {SECOND_TO_MILLISECOND}
        """,
        [expiry_base_epoch_secs],
    )

    results = db_cursor.fetchall()
    if len(results) != 0:
        archive_ids_by_dataset_id: dict[int, list[int]] = defaultdict(list)
        for result in results:
            archive_ids_by_dataset_id[result["dataset_id"]].append(result["id"])
            archive_uuid = str(uuid.UUID(bytes=result["uuid"]))
            candidates_buffer.add_candidate(f"{result['dataset']}/{archive_uuid}")

        for dataset_id, archive_ids in archive_ids_by_dataset_id.items():
            delete_archives_from_metadata_db(db_cursor, archive_ids, table_prefix, dataset_id)

        candidates_buffer.persist_new_candidates()
        db_conn.commit()

    candidates_to_delete = candidates_buffer.get_candidates()
    num_candidates_to_delete = len(candidates_to_delete)
    if 0 == num_candidates_to_delete:
        logger.debug(
            "No archives matched the expiry criteria:"
            f" `creation_time < {expiry_base_epoch_secs} - dataset's retention period`."
        )
        return

    execute_deletion(archive_output_config, candidates_to_delete)

    candidates_buffer.clear()
    logger.info(f"Deleted {num_candidates_to_delete} archive(s): {sorted(candidates_to_delete)}")


def _get_safe_expiry_base_epoch(db_cursor) -> int:
    """
    Calculates the base timestamp from which each dataset's retention period is subtracted to
    determine which of the dataset's archives have expired.

    If no query jobs are running, the base is the current time. If a query job is running and was
    created at `creation_time`, the query scheduler guarantees that it won't search any archive
    created before `creation_time - retention_period`, so the base can be safely adjusted to
    `creation_time`.

    :param db_cursor: Database cursor object
    :return: Epoch timestamp (in seconds) to subtract each dataset's retention period from.
    """
    current_epoch_secs = time.time()
    expiry_base_epoch: int

    db_cursor.execute(
        f"""
        SELECT id, creation_time
        FROM `{QUERY_JOBS_TABLE_NAME}`
        WHERE {QUERY_JOBS_TABLE_NAME}.status = {QueryJobStatus.RUNNING}
        AND {QUERY_JOBS_TABLE_NAME}.creation_time < FROM_UNIXTIME(%s)
        ORDER BY creation_time ASC
        LIMIT 1
        """,
        [current_epoch_secs],
    )

    row = db_cursor.fetchone()
    if row is not None:
        job_creation_time = row.get("creation_time")
        expiry_base_epoch = int(job_creation_time.timestamp())
        logger.debug(f"Discovered running query job created at {job_creation_time}.")
        logger.debug(f"Using adjusted expiry_base_epoch=`{expiry_base_epoch}`.")
    else:
        expiry_base_epoch = int(current_epoch_secs)
        logger.debug(f"Using expiry_base_epoch=`{expiry_base_epoch}`.")

    return expiry_base_epoch


def _collect_and_sweep_expired_archives(
    archive_output_config: ArchiveOutput,
    storage_engine: str,
    database_config: Database,
    recovery_file: pathlib.Path,
) -> None:
    candidates_buffer = DeletionCandidatesBuffer(recovery_file)

    clp_connection_param = database_config.get_clp_connection_params_and_type()
    table_prefix = clp_connection_param["table_prefix"]
    sql_adapter = SqlAdapter(database_config)
    with (
        closing(sql_adapter.create_connection(True)) as db_conn,
        closing(db_conn.cursor(dictionary=True)) as db_cursor,
    ):
        expiry_base_epoch = _get_safe_expiry_base_epoch(db_cursor)
        if StorageEngine.CLP_S != storage_engine:
            raise ValueError(f"Unsupported Storage engine: {storage_engine}.")

        _delete_expired_archives(
            db_conn,
            db_cursor,
            table_prefix,
            expiry_base_epoch,
            candidates_buffer,
            archive_output_config,
        )


async def archive_garbage_collector(clp_config: ClpConfig) -> None:
    configure_logging(logger, ARCHIVE_GARBAGE_COLLECTOR_NAME)

    archive_output_config = clp_config.archive_output
    storage_engine = clp_config.package.storage_engine
    validate_storage_type(archive_output_config, storage_engine)

    sweep_interval_secs = clp_config.garbage_collector.sweep_interval.archive * MIN_TO_SECONDS
    recovery_file = clp_config.tmp_directory / f"{ARCHIVE_GARBAGE_COLLECTOR_NAME}.tmp"

    logger.info(f"{ARCHIVE_GARBAGE_COLLECTOR_NAME} started.")
    try:
        while True:
            _collect_and_sweep_expired_archives(
                archive_output_config, storage_engine, clp_config.database, recovery_file
            )
            await asyncio.sleep(sweep_interval_secs)
    except Exception:
        logger.exception(f"{ARCHIVE_GARBAGE_COLLECTOR_NAME} exited with failure.")
        raise
