import os
from logging import Logger
from pathlib import Path

from opentelemetry.api.metrics import get_meter

from clp_py_utils.telemetry import init_telemetry
from job_orchestration.executor.utils import load_worker_config


def init_worker_telemetry(worker_name: str, logger: Logger) -> None:
    """
    Initializes OpenTelemetry metrics and emits the start event.
    """
    clp_config_path_str = os.getenv("CLP_CONFIG_PATH")
    if not clp_config_path_str:
        logger.error("CLP_CONFIG_PATH is not set; skipping telemetry initialization.")
        return

    clp_config_path = Path(clp_config_path_str)
    worker_config = load_worker_config(clp_config_path, logger)
    if not worker_config:
        logger.error("Failed to load worker config; skipping telemetry initialization.")
        return

    init_telemetry(worker_config.telemetry)

    meter = get_meter(worker_name)
    counter = meter.create_counter(
        "clp.service.event",
        description="Emitted when the service starts or stops.",
        unit="1",
    )
    counter.add(1, {"type": "start"})
