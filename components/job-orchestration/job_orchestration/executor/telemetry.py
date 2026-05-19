"""OpenTelemetry metrics initialization for CLP workers.

This module mirrors the Rust implementation in clp-rust-utils/src/telemetry.rs,
providing equivalent telemetry initialization, shutdown, and consent-checking
behaviour for Python Celery workers.

NOTE: TELEMETRY_DISABLE_VALUES must be kept consistent with the Rust
implementation in ``clp-rust-utils/src/telemetry.rs`` and the Python
implementation in ``clp_package_utils/scripts/start_clp.py``.
"""

import os

from opentelemetry.api.metrics import set_meter_provider
from opentelemetry.exporter.otlp.proto.http.metric_exporter import OTLPMetricExporter
from opentelemetry.sdk.metrics import MeterProvider
from opentelemetry.sdk.metrics.export import PeriodicExportingMetricReader
from opentelemetry.sdk.resources import Resource

# Values accepted by CLP_DISABLE_TELEMETRY and DO_NOT_TRACK to disable telemetry.
TELEMETRY_DISABLE_VALUES = ("1", "true", "yes", "y")

# Default OTLP HTTP endpoint for the OpenTelemetry Collector.
_DEFAULT_OTEL_ENDPOINT = "http://localhost:4318"

_meter_provider: MeterProvider | None = None


def is_telemetry_disabled() -> bool:
    """Check if telemetry is disabled via environment variables.

    Returns True if either ``CLP_DISABLE_TELEMETRY`` or ``DO_NOT_TRACK`` is
    set to one of :data:`TELEMETRY_DISABLE_VALUES`.
    """
    disable_telemetry = os.getenv("CLP_DISABLE_TELEMETRY")
    if disable_telemetry is not None and disable_telemetry.lower() in TELEMETRY_DISABLE_VALUES:
        return True

    do_not_track = os.getenv("DO_NOT_TRACK")
    if do_not_track is not None and do_not_track.lower() in TELEMETRY_DISABLE_VALUES:
        return True

    return False


def init_telemetry() -> None:
    """Initialize OpenTelemetry metrics collection.

    Sets the global :class:`MeterProvider` so that subsequent calls to
    :func:`opentelemetry.api.metrics.get_meter` return a real meter.  When
    telemetry is disabled (via env vars) this function is a no-op; callers
    that create counters on the resulting no-op meter will silently do
    nothing.

    The collector endpoint is read from the standard
    ``OTEL_EXPORTER_OTLP_ENDPOINT`` environment variable.  ``OTEL_SERVICE_NAME``
    and ``OTEL_RESOURCE_ATTRIBUTES`` are picked up automatically by
    :meth:`Resource.create`.
    """
    global _meter_provider

    if is_telemetry_disabled():
        return

    endpoint = os.getenv("OTEL_EXPORTER_OTLP_ENDPOINT", _DEFAULT_OTEL_ENDPOINT)
    metrics_endpoint = f"{endpoint}/v1/metrics"

    exporter = OTLPMetricExporter(endpoint=metrics_endpoint)
    reader = PeriodicExportingMetricReader(exporter)
    provider = MeterProvider(
        resource=Resource.create(),
        metric_readers=[reader],
    )

    set_meter_provider(provider)
    _meter_provider = provider


def shutdown_telemetry() -> None:
    """Shutdown OpenTelemetry metrics, flushing any pending metric exports.

    Safe to call even when telemetry was never initialized or was disabled.
    """
    global _meter_provider

    if _meter_provider is not None:
        _meter_provider.shutdown()
        _meter_provider = None
