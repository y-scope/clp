import logging

from opentelemetry import metrics
from opentelemetry.exporter.otlp.proto.http.metric_exporter import OTLPMetricExporter
from opentelemetry.metrics import NoOpMeterProvider
from opentelemetry.sdk.metrics import MeterProvider
from opentelemetry.sdk.metrics.export import PeriodicExportingMetricReader
from opentelemetry.sdk.metrics.view import ExplicitBucketHistogramAggregation, View

from clp_py_utils.telemetry_config import is_telemetry_disabled_by_env

logger = logging.getLogger(__name__)


_QUERY_BYTES_SCANNED_HISTOGRAM_BOUNDARIES = (
    0,
    1 << 20,  # 1 MiB
    1 << 22,  # 4 MiB
    1 << 24,  # 16 MiB
    1 << 26,  # 64 MiB
    1 << 28,  # 256 MiB
    1 << 30,  # 1 GiB
    1 << 32,  # 4 GiB
    1 << 34,  # 16 GiB
    1 << 36,  # 64 GiB
    1 << 38,  # 256 GiB
    1 << 40,  # 1 TiB
    1 << 42,  # 4 TiB
)


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

        query_uncompressed_bytes_scanned_view = View(
            instrument_name="clp.query.uncompressed_bytes_scanned",
            aggregation=ExplicitBucketHistogramAggregation(
                boundaries=_QUERY_BYTES_SCANNED_HISTOGRAM_BOUNDARIES
            ),
        )
        query_compressed_bytes_scanned_view = View(
            instrument_name="clp.query.compressed_bytes_scanned",
            aggregation=ExplicitBucketHistogramAggregation(
                boundaries=_QUERY_BYTES_SCANNED_HISTOGRAM_BOUNDARIES
            ),
        )

        provider = MeterProvider(
            metric_readers=[reader],
            views=[
                query_uncompressed_bytes_scanned_view,
                query_compressed_bytes_scanned_view,
            ],
        )
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
