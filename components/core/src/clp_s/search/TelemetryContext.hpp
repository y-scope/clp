#ifndef CLP_S_SEARCH_TELEMETRYCONTEXT_HPP
#define CLP_S_SEARCH_TELEMETRYCONTEXT_HPP

#include <memory>

namespace clp_s::search {
/**
 * RAII guard that configures the process-global OpenTelemetry tracer provider.
 *
 * On construction it installs a `TracerProvider` backed by a batch span processor and an OTLP HTTP
 * exporter so that the spans emitted by `SearchTelemetrySpan` are actually exported. On
 * destruction it flushes any buffered spans and shuts the provider down, which is required for a
 * short-lived CLI process to avoid dropping telemetry on exit.
 */
class TelemetryContext {
public:
    // Constructors
    TelemetryContext();

    // Delete copy constructor and assignment operator
    TelemetryContext(TelemetryContext const&) = delete;
    auto operator=(TelemetryContext const&) -> TelemetryContext& = delete;

    // Delete move constructor and assignment operator
    TelemetryContext(TelemetryContext&&) = delete;
    auto operator=(TelemetryContext&&) -> TelemetryContext& = delete;

    // Destructor
    ~TelemetryContext() = default;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_TELEMETRYCONTEXT_HPP
