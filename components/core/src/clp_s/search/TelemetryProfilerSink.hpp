#ifndef CLP_S_SEARCH_TELEMETRYPROFILERSINK_HPP
#define CLP_S_SEARCH_TELEMETRYPROFILERSINK_HPP

#include <memory>
#include <string_view>
#include <utility>

#include <utils/profiling/Sink.hpp>

#include "SearchTelemetry.hpp"

namespace clp_s::search {
/**
 * Sink that writes profiler measurements to a `SearchTelemetrySpan` as span attributes.
 */
class TelemetryProfilerSink : public utils::profiling::Sink {
public:
    // Constructors
    /**
     * @param telemetry_span The span to record attributes on.
     */
    explicit TelemetryProfilerSink(std::shared_ptr<SearchTelemetrySpan> telemetry_span)
            : m_telemetry_span{std::move(telemetry_span)} {}

    // Methods implementing utils::profiling::Sink
    /**
     * Emits the measurement on the telemetry span.
     */
    auto emit(std::string_view name, utils::profiling::Measurement measurement) -> void override {
        m_telemetry_span->set_profiler_measurement(name, measurement);
    }

private:
    // Data members
    std::shared_ptr<SearchTelemetrySpan> m_telemetry_span;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_TELEMETRYPROFILERSINK_HPP
