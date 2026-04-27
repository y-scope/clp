from logging import Logger
from pathlib import Path

from clp_py_utils.clp_config import WorkerConfig
from clp_py_utils.core import read_yaml_config_file
import os
from opentelemetry import metrics
from opentelemetry.exporter.otlp.proto.http.metric_exporter import OTLPMetricExporter
from opentelemetry.sdk.metrics import MeterProvider
from opentelemetry.sdk.metrics.export import PeriodicExportingMetricReader
from opentelemetry.sdk.resources import Resource

_OTEL_INITIALIZED = False

def init_otel(service_name: str) -> None:
    global _OTEL_INITIALIZED
    if _OTEL_INITIALIZED:
        return
    _OTEL_INITIALIZED = True
    
    if os.environ.get("OTEL_SDK_DISABLED", "false").lower() == "true":
        return
        
    resource = Resource.create({"service.name": service_name})
    reader = PeriodicExportingMetricReader(OTLPMetricExporter())
    provider = MeterProvider(resource=resource, metric_readers=[reader])
    metrics.set_meter_provider(provider)


def load_worker_config(
    config_path: Path,
    logger: Logger,
) -> WorkerConfig | None:
    """
    Loads a WorkerConfig object from the specified configuration file.
    :param config_path: Path to the configuration file.
    :param logger: Logger instance for reporting errors if loading fails.
    :return: The loaded WorkerConfig object on success, None otherwise.
    """
    try:
        return WorkerConfig.model_validate(read_yaml_config_file(config_path))
    except Exception:
        logger.exception("Failed to load worker config")
        return None
