import logging
import os

from opentelemetry import metrics
from opentelemetry.exporter.otlp.proto.http.metric_exporter import OTLPMetricExporter
from opentelemetry.metrics import NoOpMeterProvider
from opentelemetry.sdk.metrics import MeterProvider
from opentelemetry.sdk.metrics.export import PeriodicExportingMetricReader

logger = logging.getLogger(__name__)

TELEMETRY_DISABLE_VALUES = {"1", "true", "yes", "y"}


def init_telemetry() -> None:
    """
    Initializes OpenTelemetry metrics collection.

    If telemetry is disabled via environment variables (CLP_DISABLE_TELEMETRY or DO_NOT_TRACK),
    this function does nothing.
    """
    disable_env_var = os.environ.get("CLP_DISABLE_TELEMETRY", "").strip().lower()
    dnt_env_var = os.environ.get("DO_NOT_TRACK", "").strip().lower()
    if disable_env_var in TELEMETRY_DISABLE_VALUES or dnt_env_var in TELEMETRY_DISABLE_VALUES:
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
        logger.warning(f"Failed to initialize OpenTelemetry metrics: {e}.")


def shutdown_telemetry() -> None:
    """
    Shuts down the meter provider, flushing any pending metric exports.
    """
    provider = metrics.get_meter_provider()
    if hasattr(provider, "force_flush"):
        try:
            provider.force_flush()
        except Exception as e:
            logger.warning(f"Failed to force flush OpenTelemetry metrics: {e}.")

    if hasattr(provider, "shutdown"):
        try:
            provider.shutdown()
        except Exception as e:
            logger.warning(f"Failed to shut down OpenTelemetry metrics: {e}.")
