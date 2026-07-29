import logging

from opentelemetry import metrics
from opentelemetry.exporter.otlp.proto.http.metric_exporter import OTLPMetricExporter
from opentelemetry.metrics import NoOpMeterProvider
from opentelemetry.sdk.metrics import MeterProvider
from opentelemetry.sdk.metrics.export import PeriodicExportingMetricReader

from clp_py_utils.telemetry_config import is_telemetry_disabled_by_env

logger = logging.getLogger(__name__)


def init_telemetry() -> None:
    """
    Initializes OpenTelemetry metrics collection.

    If telemetry is disabled via environment variables (CLP_DISABLE_TELEMETRY or DO_NOT_TRACK),
    this function installs a NoOpMeterProvider so that any metrics API calls become no-ops.
    """
    if is_telemetry_disabled_by_env():
        metrics.set_meter_provider(NoOpMeterProvider())
        logger.debug("OpenTelemetry metrics disabled.")
        return

    try:
        exporter = OTLPMetricExporter()
        reader = PeriodicExportingMetricReader(exporter)
        provider = MeterProvider(metric_readers=[reader])
        metrics.set_meter_provider(provider)
        logger.debug("OpenTelemetry metrics initialized successfully.")
    except Exception as e:
        logger.warning("Failed to initialize OpenTelemetry metrics: %s.", e)


def shutdown_telemetry() -> None:
    """Shuts down the meter provider, flushing any pending metric exports."""
    provider = metrics.get_meter_provider()
    if hasattr(provider, "force_flush"):
        try:
            provider.force_flush()
        except Exception as e:
            logger.warning("Failed to force flush OpenTelemetry metrics: %s.", e)

    if hasattr(provider, "shutdown"):
        try:
            provider.shutdown()
        except Exception as e:
            logger.warning("Failed to shut down OpenTelemetry metrics: %s.", e)
