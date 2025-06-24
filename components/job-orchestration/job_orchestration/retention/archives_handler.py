import pathlib
import time
from contextlib import closing
from typing import List, Optional

from clp_py_utils.clp_config import ArchiveOutput, CLPConfig, StorageEngine
from clp_py_utils.clp_logging import get_logger, get_logging_formatter, set_logging_level
from clp_py_utils.clp_metadata_db_utils import (
    delete_archives_from_metadata_db,
    fetch_existing_datasets,
    get_archives_table_name,
)
from clp_py_utils.constants import SECOND_TO_MILLISECOND
from clp_py_utils.sql_adapter import SQL_Adapter
from job_orchestration.retention.utils import (
    get_target_time,
    remove_targets,
    TargetsBuffer,
)

logger = get_logger("ARCHIVES_RETENTION_HANDLER")


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


def handle_archive_retention(clp_config: CLPConfig) -> None:
    archive_output_config = clp_config.archive_output
    storage_engine = clp_config.package.storage_engine
    archive_expiry_epoch = SECOND_TO_MILLISECOND * get_target_time(
        archive_output_config.retention_period
    )

    logger.info(f"Handler targeting all archives with end_ts < {archive_expiry_epoch}")

    logger.info("Creating SQL adapter")
    sql_adapter = SQL_Adapter(clp_config.database)
    clp_connection_param = clp_config.database.get_clp_connection_params_and_type()
    table_prefix = clp_connection_param["table_prefix"]

    recovery_file = clp_config.logs_directory / "archive_retention.tmp"
    logger.info("Creating target handle")
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


def archive_retention_entry(clp_config: CLPConfig, job_frequency_secs: int, input_logger) -> None:
    global logger
    logger = input_logger
    archive_retention_period = clp_config.archive_output.retention_period
    if archive_retention_period is None:
        logger.info("No archive retention period is specified, give up")
        return
    while True:
        handle_archive_retention(clp_config)
        time.sleep(job_frequency_secs)
