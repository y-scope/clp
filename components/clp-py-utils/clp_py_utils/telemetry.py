"""OpenTelemetry metrics initialization for CLP Python components.

This module mirrors the Rust implementation, providing equivalent telemetry
initialization, shutdown, and consent-checking behaviour.
"""

import atexit
import logging
import os
from typing import Optional

from opentelemetry import metrics
from opentelemetry.exporter.otlp.proto.http.metric_exporter import OTLPMetricExporter
from opentelemetry.sdk.metrics import MeterProvider
from opentelemetry.sdk.metrics.export import PeriodicExportingMetricReader
from opentelemetry.sdk.resources import Resource

from clp_py_utils.clp_config import Telemetry

logger = logging.getLogger(__name__)

# Values accepted by CLP_DISABLE_TELEMETRY and DO_NOT_TRACK to disable telemetry.
TELEMETRY_DISABLE_VALUES = ("1", "true", "yes", "y")

# Default OTLP HTTP endpoint for the OpenTelemetry Collector.
_DEFAULT_OTEL_ENDPOINT = "http://localhost:4318"

_meter_provider: Optional[MeterProvider] = None


def is_telemetry_disabled(telemetry_config: Telemetry) -> bool:
    """Check if telemetry is disabled via config or environment variables."""
    if telemetry_config.disable:
        return True

    disable_telemetry = os.getenv("CLP_DISABLE_TELEMETRY")
    if disable_telemetry is not None and disable_telemetry.lower() in TELEMETRY_DISABLE_VALUES:
        return True

    do_not_track = os.getenv("DO_NOT_TRACK")
    if do_not_track is not None and do_not_track.lower() in TELEMETRY_DISABLE_VALUES:
        return True

    return False


def init_telemetry(telemetry_config: Telemetry) -> None:
    """Initialize OpenTelemetry metrics collection."""
    global _meter_provider

    if _meter_provider is not None:
        return

    if is_telemetry_disabled(telemetry_config):
        return

    endpoint = os.getenv("OTEL_EXPORTER_OTLP_ENDPOINT", _DEFAULT_OTEL_ENDPOINT)
    metrics_endpoint = f"{endpoint}/v1/metrics"

    exporter = OTLPMetricExporter(endpoint=metrics_endpoint)
    reader = PeriodicExportingMetricReader(exporter)
    provider = MeterProvider(
        resource=Resource.create(),
        metric_readers=[reader],
    )

    metrics.set_meter_provider(provider)
    _meter_provider = provider

    # Ensure clean shutdown
    atexit.register(shutdown_telemetry)


def shutdown_telemetry() -> None:
    """Shutdown OpenTelemetry metrics, flushing any pending metric exports."""
    global _meter_provider
    if _meter_provider is not None:
        try:
            _meter_provider.shutdown()
        except Exception as e:
            logger.error(f"Failed to shutdown telemetry: {e}")
        _meter_provider = None
