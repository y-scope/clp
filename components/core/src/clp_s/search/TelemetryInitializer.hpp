#ifndef CLP_S_SEARCH_TELEMETRYINITIALIZER_HPP
#define CLP_S_SEARCH_TELEMETRYINITIALIZER_HPP

#include <memory>

namespace clp_s::search {
/**
 * RAII guard that configures the process-global OpenTelemetry tracer provider.
 *
 * On construction it installs a `TracerProvider` backed by a batch span processor and an OTLP HTTP
 * exporter so that the spans emitted by `SearchTelemetrySpan` are actually exported. On destruction
 * it force-flushes any buffered spans and shuts the provider down, which is required for a
 * short-lived CLI process to avoid dropping telemetry on exit.
 *
 * Telemetry is left disabled (the provider remains the API's default no-op, so spans become no-ops)
 * when either:
 * - `CLP_DISABLE_TELEMETRY` or `DO_NOT_TRACK` is set to a truthy value (`1`, `true`, `yes`, `y`).
 * - The provider/exporter could not be constructed.
 *
 * The exporter endpoint is resolved from (in order of precedence) the standard
 * `OTEL_EXPORTER_OTLP_TRACES_ENDPOINT`/`OTEL_EXPORTER_OTLP_ENDPOINT` environment variables, and
 * finally CLP's `CLP_TELEMETRY_ENDPOINT`. Resource attributes are picked up from
 * `OTEL_SERVICE_NAME` and `OTEL_RESOURCE_ATTRIBUTES` per the OpenTelemetry specification.
 */
class TelemetryContext {
public:
    // Constructors
    TelemetryContext();

    // Disable copy/move constructors and assignment operators
    TelemetryContext(TelemetryContext const&) = delete;
    TelemetryContext(TelemetryContext&&) = delete;
    auto operator=(TelemetryContext const&) -> TelemetryContext& = delete;
    auto operator=(TelemetryContext&&) -> TelemetryContext& = delete;

    // Destructor
    ~TelemetryContext();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_TELEMETRYINITIALIZER_HPP
