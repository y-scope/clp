import logging
import os
import subprocess
from pathlib import Path
from typing import Any, Dict

import yaml
from celery.app.task import Task
from celery.utils.log import get_task_logger
from celery.utils.nodenames import gethostname  # type: ignore

from job_orchestration.executor.compression.celery import app  # type: ignore

from clp.package_utils import CONTAINER_INPUT_LOGS_ROOT_DIR

celery_central_logger = get_task_logger(__name__)
celery_central_logger.setLevel(logging.INFO)


@app.task(bind=True)
def compress(
    self: Task,
    job_id_str: str,
    # V0.5 TODO: remove this two
    db_config: Dict[str, Any],
    job_input_config: Dict[str, Any],
    job_output_config: Dict[str, Any],
    clp_db_config: Dict[str, Any],
    paths_to_compress: Dict[str, Any],
) -> bool:
    clp_home = Path(os.getenv("CLP_HOME"))
    celery_logs_dir = Path(os.getenv("CLP_LOGS_DIR"))

    task_id_str = self.request.id
    celery_central_logger.info(f"Started job {job_id_str}. Task Id={task_id_str}.")

    # Setup logging and data folder
    celery_data_dir = Path("/") / "tmp"

    # Create logger
    logger = logging.getLogger()
    logger.setLevel(logging.INFO)
    # Setup logging to file
    script_log_path = celery_logs_dir / f"{task_id_str}-worker-script.log"
    celery_logging_file_handler = logging.FileHandler(filename=script_log_path, encoding="utf-8")
    celery_logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
    celery_logging_file_handler.setFormatter(celery_logging_formatter)
    logger.addHandler(celery_logging_file_handler)

    # Generate database config file
    clp_db_config_file_path = celery_data_dir / f"{task_id_str}-db-config.yml"
    with open(clp_db_config_file_path, "w") as f:
        yaml.safe_dump(clp_db_config, f)

    # Expand parameters
    file_paths = paths_to_compress["file_paths"]
    empty_directories = paths_to_compress.get("empty_directories")
    archives_dir = Path(os.getenv("CLP_ARCHIVE_OUTPUT_DIR"))
    archives_dir.mkdir(parents=True, exist_ok=True)

    # TODO This depends on the input paths being absolute and assumes we're
    # running in a container
    # Prepare list of paths to compress for clp
    log_list_path = celery_data_dir / f"{task_id_str}-log-paths.txt"
    with open(log_list_path, "w") as f:
        for path_str in file_paths:
            path_on_host = Path(path_str)
            file_path = CONTAINER_INPUT_LOGS_ROOT_DIR / path_on_host.relative_to(
                path_on_host.anchor)
            f.write(f"{file_path}\n")

        # Add empty directories if any
        if empty_directories is not None and len(empty_directories) > 0:
            for path_str in empty_directories:
                path_on_host = Path(path_str)
                dir_path = CONTAINER_INPUT_LOGS_ROOT_DIR / path_on_host.relative_to(
                    path_on_host.anchor)
                f.write(f"{dir_path}\n")

    # Assemble compression command
    # fmt: off
    compression_cmd = [
        str(clp_home / "bin" / "clp"),
        "c", str(archives_dir),
        "--print-archive-stats-progress",
        "--target-dictionaries-size",
        str(job_output_config["target_archive_dictionaries_data_size"]),
        "--target-segment-size", str(job_output_config["target_segment_size"]),
        "--target-encoded-file-size", str(job_output_config["target_encoded_file_size"]),
        "--db-config-file", str(clp_db_config_file_path),
        "--files-from", str(log_list_path),
        # TODO Remove when we can run outside the container
        "--remove-path-prefix", str(CONTAINER_INPUT_LOGS_ROOT_DIR)
    ]
    # fmt: on
    logger.info(compression_cmd)

    # Open stderr log file
    stderr_log_path = celery_logs_dir / f"{task_id_str}-stderr.log"
    stderr_log_file = open(stderr_log_path, "w")

    # Start compression
    logger.info("Compressing...")
    compression_successful = False
    proc = subprocess.Popen(
        compression_cmd,
        close_fds=True,
        stdout=subprocess.PIPE,
        stderr=stderr_log_file,
    )

    # Wait for compression to finish
    return_code = proc.wait()
    if 0 != return_code:
        logger.info(f"Failed to compress, return_code={return_code}")
    else:
        compression_successful = True

        # Remove path lists
        log_list_path.unlink()
    logger.info("Compressed.")

    # Close log files
    stderr_log_file.close()
    logger.removeHandler(celery_logging_file_handler)
    celery_logging_file_handler.close()

    return compression_successful