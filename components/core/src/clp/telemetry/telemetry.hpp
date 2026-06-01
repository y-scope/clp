#ifndef CLP_TELEMETRY_HPP
#define CLP_TELEMETRY_HPP

#include <cstdint>
#include <string>

namespace clp::telemetry {

/**
 * Initializes OpenTelemetry globally.
 * Must be called once at the start of the program.
 */
void init();

/**
 * Shuts down OpenTelemetry globally, flushing metrics.
 */
void shutdown();

/**
 * Records query metrics.
 * @param bytes_scanned The total uncompressed bytes scanned.
 * @param bytes_output The total bytes outputted.
 */
void record_query_metrics(uint64_t bytes_scanned, uint64_t bytes_output);

} // namespace clp::telemetry

#endif // CLP_TELEMETRY_HPP
