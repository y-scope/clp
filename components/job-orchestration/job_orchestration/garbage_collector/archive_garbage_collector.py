import asyncio
import pathlib
import time
from contextlib import closing

from clp_py_utils.clp_config import (
    ArchiveOutput,
    ClpConfig,
    Database,
    QUERY_JOBS_TABLE_NAME,
    StorageEngine,
)
from clp_py_utils.clp_logging import get_logger
from clp_py_utils.clp_metadata_db_utils import (
    delete_archives_from_metadata_db,
    fetch_existing_datasets,
    get_archives_table_name,
)
from clp_py_utils.sql_adapter import SqlAdapter

from job_orchestration.garbage_collector.constants import (
    ARCHIVE_GARBAGE_COLLECTOR_NAME,
    MIN_TO_SECONDS,
    SECOND_TO_MILLISECOND,
)
from job_orchestration.garbage_collector.utils import (
    configure_logger,
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
    archive_expiry_epoch_secs: int,
    candidates_buffer: DeletionCandidatesBuffer,
    archive_output_config: ArchiveOutput,
    dataset: str | None,
) -> None:
    archives_table = get_archives_table_name(table_prefix, dataset)
    archive_end_ts_upper_bound = archive_expiry_epoch_secs * SECOND_TO_MILLISECOND

    db_cursor.execute(
        f"""
        SELECT id FROM `{archives_table}`
        WHERE end_timestamp < %s
        AND end_timestamp != 0
        """,
        [archive_end_ts_upper_bound],
    )

    results = db_cursor.fetchall()
    archive_ids = [result["id"] for result in results]
    if len(archive_ids) != 0:
        delete_archives_from_metadata_db(db_cursor, archive_ids, table_prefix, dataset)

        for candidate in archive_ids:
            if dataset is not None:
                candidate = f"{dataset}/{candidate}"
            candidates_buffer.add_candidate(candidate)

        candidates_buffer.persist_new_candidates()
        db_conn.commit()

    candidates_to_delete = candidates_buffer.get_candidates()
    num_candidates_to_delete = len(candidates_to_delete)
    if 0 == num_candidates_to_delete:
        logger.debug(
            f"No archives matched the expiry criteria: `end_ts < {archive_end_ts_upper_bound}`."
        )
        return

    execute_deletion(archive_output_config, candidates_to_delete)

    # Prepare the log message
    dataset_msg: str
    deleted_candidates: list[str]
    if dataset is not None:
        dataset_log_msg = f" from dataset `{dataset}`"
        # Note: If dataset is not None, candidates are expected to be in the format
        # `<dataset>/<archive_id>`
        deleted_candidates = [candidate.split("/")[1] for candidate in candidates_to_delete]
    else:
        dataset_log_msg = ""
        deleted_candidates = list(candidates_to_delete)

    candidates_buffer.clear()
    logger.info(
        f"Deleted {num_candidates_to_delete} archive(s){dataset_log_msg}: {deleted_candidates}"
    )


def _get_archive_safe_expiry_epoch(
    db_cursor,
    retention_period_minutes: int,
) -> int:
    """
    Calculates a safe expiration timestamp such that archives with `end_ts` less than this value are
    guaranteed not to be searched by any running query jobs.

    If no query jobs are running, the expiry time is set to `current_time - retention_period`.
    If a query job is running and was created at `creation_time`, the query scheduler guarantees
    that it will not search any archive whose end_ts < (creation_time - retention_period).
    In this case, the expiry time can be safely adjusted to `creation_time - retention_period`.

    Note: This function does not consider query jobs that started before
    `current_time - retention_period`, as such long-running jobs are likely hanging. Including them
    would prevent the expiry time from advancing.

    :param db_cursor: Database cursor object
    :param retention_period_minutes: Retention window in minutes
    :return: Epoch timestamp indicating the safe expiration time (in seconds)
    """
    retention_period_secs = retention_period_minutes * MIN_TO_SECONDS
    current_epoch_secs = time.time()
    archive_expiry_epoch: int

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
        archive_expiry_epoch = int(job_creation_time.timestamp()) - retention_period_secs
        logger.debug(f"Discovered running query job created at {job_creation_time}.")
        logger.debug(f"Using adjusted archive_expiry_epoch=`{archive_expiry_epoch}`.")
    else:
        archive_expiry_epoch = int(current_epoch_secs) - retention_period_secs
        logger.debug(f"Using archive_expiry_epoch=`{archive_expiry_epoch}`.")

    return archive_expiry_epoch


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
        archive_expiry_epoch = _get_archive_safe_expiry_epoch(
            db_cursor,
            archive_output_config.retention_period,
        )
        if StorageEngine.CLP_S == storage_engine:
            datasets = fetch_existing_datasets(db_cursor, table_prefix)
            for dataset in datasets:
                logger.debug(f"Running garbage collection on dataset `{dataset}`")
                _delete_expired_archives(
                    db_conn,
                    db_cursor,
                    table_prefix,
                    archive_expiry_epoch,
                    candidates_buffer,
                    archive_output_config,
                    dataset,
                )
        elif StorageEngine.CLP == storage_engine:
            _delete_expired_archives(
                db_conn,
                db_cursor,
                table_prefix,
                archive_expiry_epoch,
                candidates_buffer,
                archive_output_config,
                None,
            )
        else:
            raise ValueError(f"Unsupported Storage engine: {storage_engine}.")


async def archive_garbage_collector(
    clp_config: ClpConfig, log_directory: pathlib.Path, logging_level: str
) -> None:
    configure_logger(logger, logging_level, log_directory, ARCHIVE_GARBAGE_COLLECTOR_NAME)

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
