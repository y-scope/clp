import os
from logging import Logger
from pathlib import Path

from clp_py_utils.clp_config import WorkerConfig
from clp_py_utils.core import read_yaml_config_file
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

    import platform
    resource = Resource.create({
        "service.name": service_name,
        "telemetry.id": os.environ.get("CLP_INSTANCE_ID", "unknown"),
        "clp.version": os.environ.get("CLP_VERSION", "unknown"),
        "deployment.method": os.environ.get("CLP_DEPLOYMENT_METHOD", "unknown"),
        "os.type": os.environ.get("CLP_HOST_OS", platform.system().lower()),
        "os.version": os.environ.get("CLP_HOST_OS_VERSION", "unknown"),
        "host.arch": os.environ.get("CLP_HOST_ARCH", platform.machine().lower()),
        "clp.storage_engine": os.environ.get("CLP_STORAGE_ENGINE", "clp"),
    })
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
