import logging
import pathlib
import time
from contextlib import closing
from typing import List, Optional

from clp_py_utils.clp_config import ArchiveOutput, CLPConfig, Database, StorageEngine
from clp_py_utils.clp_logging import get_logger, get_logging_formatter, set_logging_level
from clp_py_utils.clp_metadata_db_utils import (
    delete_archives_from_metadata_db,
    fetch_existing_datasets,
    get_archives_table_name,
)
from clp_py_utils.constants import MIN_TO_SECONDS, SECOND_TO_MILLISECOND
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.retention.utils import (
    configure_logger,
    get_expiry_epoch_secs,
    remove_targets,
    TargetsBuffer,
    validate_storage_type,
)

HANDLER_NAME = "archive_retention_handler"
logger = get_logger(HANDLER_NAME)


def archive_retention_helper(
    db_conn,
    db_cursor,
    table_prefix: str,
    archive_expiry_epoch: int,
    targets_buffer: TargetsBuffer,
    archive_output_config: ArchiveOutput,
    dataset: Optional[str],
):
    # Remove all archives
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
    archive_ids: List[str] = [result["id"] for result in results]
    if len(archive_ids) != 0:
        logger.info(f"Deleting {archive_ids}")
        delete_archives_from_metadata_db(db_cursor, archive_ids, table_prefix, dataset)

        for target in archive_ids:
            if dataset is not None:
                target = f"{dataset}/{target}"
            targets_buffer.add_target(target)

        targets_buffer.persists_new_targets()
        db_conn.commit()

    remove_targets(archive_output_config, targets_buffer.get_targets())
    targets_buffer.flush()


def handle_archive_retention(
    archive_output_config: ArchiveOutput,
    storage_engine: str,
    database_config: Database,
    clp_logs_directory: pathlib.Path,
) -> None:
    archive_expiry_epoch = SECOND_TO_MILLISECOND * get_expiry_epoch_secs(
        archive_output_config.retention_period
    )

    logger.info(f"Handler targeting all archives with end_ts < {archive_expiry_epoch}")
    sql_adapter = SQL_Adapter(database_config)
    clp_connection_param = database_config.get_clp_connection_params_and_type()
    table_prefix = clp_connection_param["table_prefix"]

    recovery_file = clp_logs_directory / f"{HANDLER_NAME}.tmp"
    targets_buffer = TargetsBuffer(recovery_file)

    logger.info("mysql cursor starts")
    with closing(sql_adapter.create_connection(True)) as db_conn, closing(
        db_conn.cursor(dictionary=True)
    ) as db_cursor:
        if StorageEngine.CLP_S == storage_engine:
            datasets = fetch_existing_datasets(db_cursor, table_prefix)
            for dataset in datasets:
                archive_retention_helper(
                    db_conn,
                    db_cursor,
                    table_prefix,
                    archive_expiry_epoch,
                    targets_buffer,
                    archive_output_config,
                    dataset,
                )
        elif StorageEngine.CLP == storage_engine:
            archive_retention_helper(
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


def archive_retention_entry(
    clp_config: CLPConfig, log_directory: pathlib, logging_level: str
) -> None:
    configure_logger(logger, logging_level, log_directory, HANDLER_NAME)

    job_frequency_minutes = clp_config.retention_daemon.job_frequency.archives
    archive_retention_period = clp_config.archive_output.retention_period
    if archive_retention_period is None:
        logger.info("Archive retention period is not specified, terminate")
        return
    if job_frequency_minutes is None:
        logger.info("Job frequency is not specified, terminate")
        return

    archive_output_config: ArchiveOutput = clp_config.archive_output
    storage_engine: str = clp_config.package.storage_engine
    validate_storage_type(archive_output_config, storage_engine)

    while True:
        handle_archive_retention(
            archive_output_config, storage_engine, clp_config.database, clp_config.logs_directory
        )
        time.sleep(job_frequency_minutes * MIN_TO_SECONDS)
