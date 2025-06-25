import asyncio
import pathlib
from contextlib import closing
from typing import Optional

from clp_py_utils.clp_config import ArchiveOutput, CLPConfig, Database, StorageEngine
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
    get_expiry_epoch_secs,
    remove_targets,
    TargetsBuffer,
    validate_storage_type,
)

logger = get_logger(ARCHIVES_RETENTION_HANDLER_NAME)


def _remove_expired_archives(
    db_conn,
    db_cursor,
    table_prefix: str,
    archive_expiry_epoch: int,
    targets_buffer: TargetsBuffer,
    archive_output_config: ArchiveOutput,
    dataset: Optional[str],
):
    archives_table = get_archives_table_name(table_prefix, dataset)
    db_cursor.execute(
        f"""
        SELECT id FROM `{archives_table}`
        WHERE end_timestamp <= %s
        AND end_timestamp != 0
        """,
        [archive_expiry_epoch],
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


def _handle_archive_retention(
    archive_output_config: ArchiveOutput,
    storage_engine: str,
    database_config: Database,
    clp_logs_directory: pathlib.Path,
) -> None:
    archive_expiry_epoch = SECOND_TO_MILLISECOND * get_expiry_epoch_secs(
        archive_output_config.retention_period
    )

    clp_connection_param = database_config.get_clp_connection_params_and_type()
    table_prefix = clp_connection_param["table_prefix"]

    recovery_file = clp_logs_directory / f"{ARCHIVES_RETENTION_HANDLER_NAME}.tmp"
    targets_buffer = TargetsBuffer(recovery_file)

    sql_adapter = SQL_Adapter(database_config)
    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
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
    clp_config: CLPConfig, log_directory: pathlib, logging_level: str
) -> None:
    configure_logger(logger, logging_level, log_directory, ARCHIVES_RETENTION_HANDLER_NAME)

    archive_output_config = clp_config.archive_output
    storage_engine = clp_config.package.storage_engine
    validate_storage_type(archive_output_config, storage_engine)

    job_frequency_minutes = clp_config.retention_cleaner.job_frequency.archives
    while True:
        _handle_archive_retention(
            archive_output_config, storage_engine, clp_config.database, clp_config.logs_directory
        )
        await asyncio.sleep(job_frequency_minutes * MIN_TO_SECONDS)
