import asyncio
import pathlib
import time
from contextlib import closing
from typing import Optional

from clp_py_utils.clp_config import (
    ArchiveOutput,
    CLPConfig,
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
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.retention.constants import (
    ARCHIVES_RETENTION_HANDLER_NAME,
    MIN_TO_SECONDS,
    SECOND_TO_MILLISECOND,
)
from job_orchestration.retention.utils import (
    configure_logger,
    remove_targets,
    TargetsBuffer,
    validate_storage_type,
)
from job_orchestration.scheduler.constants import QueryJobStatus

logger = get_logger(ARCHIVES_RETENTION_HANDLER_NAME)


def _remove_expired_archives(
    db_conn,
    db_cursor,
    table_prefix: str,
    archive_expiry_epoch_secs: int,
    targets_buffer: TargetsBuffer,
    archive_output_config: ArchiveOutput,
    dataset: Optional[str],
):
    archives_table = get_archives_table_name(table_prefix, dataset)
    target_archive_end_ts = archive_expiry_epoch_secs * SECOND_TO_MILLISECOND
    logger.debug(f"Searching for archives with end_ts < {target_archive_end_ts}")
    db_cursor.execute(
        f"""
        SELECT id FROM `{archives_table}`
        WHERE end_timestamp <= %s
        AND end_timestamp != 0
        """,
        [target_archive_end_ts],
    )

    results = db_cursor.fetchall()
    archive_ids = [result["id"] for result in results]
    if len(archive_ids) != 0:
        logger.debug(f"Deleting {archive_ids}")
        delete_archives_from_metadata_db(db_cursor, archive_ids, table_prefix, dataset)

        for target in archive_ids:
            if dataset is not None:
                target = f"{dataset}/{target}"
            targets_buffer.add_target(target)

        targets_buffer.persists_new_targets()
        db_conn.commit()

    remove_targets(archive_output_config, targets_buffer.get_targets())
    targets_buffer.flush()


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
    `current_time - retention_period`, as such long-running jobs are likely hanging.
    Including them would prevent the expiry time from advancing.

    :param db_cursor: Database cursor object
    :param retention_period_minutes: Retention window in minutes
    :return: Epoch timestamp indicating the safe expiration time (in seconds)
    """
    retention_period_secs = retention_period_minutes * MIN_TO_SECONDS
    current_epoch_secs = time.time()
    archive_expiry_epoch = current_epoch_secs - retention_period_secs

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
        logger.debug(f"Discovered running query job created at {job_creation_time}")
        archive_expiry_epoch = int(job_creation_time.timestamp()) - retention_period_secs
    else:
        archive_expiry_epoch = int(current_epoch_secs) - retention_period_secs

    return archive_expiry_epoch



def _handle_archive_retention(
    archive_output_config: ArchiveOutput,
    storage_engine: str,
    database_config: Database,
    recovery_file: pathlib.Path,
) -> None:
    targets_buffer = TargetsBuffer(recovery_file)

    clp_connection_param = database_config.get_clp_connection_params_and_type()
    table_prefix = clp_connection_param["table_prefix"]
    sql_adapter = SQL_Adapter(database_config)
    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        archive_expiry_epoch = _get_archive_safe_expiry_epoch(
            db_cursor,
            archive_output_config.retention_period,
        )
        if StorageEngine.CLP_S == storage_engine:
            datasets = fetch_existing_datasets(db_cursor, table_prefix)
            for dataset in datasets:
                _remove_expired_archives(
                    db_conn,
                    db_cursor,
                    table_prefix,
                    archive_expiry_epoch,
                    targets_buffer,
                    archive_output_config,
                    dataset,
                )
        elif StorageEngine.CLP == storage_engine:
            _remove_expired_archives(
                db_conn,
                db_cursor,
                table_prefix,
                archive_expiry_epoch,
                targets_buffer,
                archive_output_config,
                None,
            )
        else:
            raise ValueError(f"Unsupported Storage engine: {storage_engine}")


async def archive_retention(
    clp_config: CLPConfig, log_directory: pathlib.Path, logging_level: str
) -> None:
    configure_logger(logger, logging_level, log_directory, ARCHIVES_RETENTION_HANDLER_NAME)

    archive_output_config = clp_config.archive_output
    storage_engine = clp_config.package.storage_engine
    validate_storage_type(archive_output_config, storage_engine)

    job_frequency_secs = clp_config.retention_cleaner.job_frequency.archives * MIN_TO_SECONDS
    recovery_file = clp_config.logs_directory / f"{ARCHIVES_RETENTION_HANDLER_NAME}.tmp"

    # Start retention loop
    while True:
        _handle_archive_retention(
            archive_output_config, storage_engine, clp_config.database, recovery_file
        )
        await asyncio.sleep(job_frequency_secs)
