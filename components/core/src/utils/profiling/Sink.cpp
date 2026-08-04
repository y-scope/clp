#include <chrono>
#include <string_view>

#include <spdlog/spdlog.h>
#include <utils/profiling/Sink.hpp>
#include <utils/profiling/Stopwatch.hpp>

namespace utils::profiling {
auto SpdlogSink::emit(std::string_view name, Measurement measurement) -> void {
    SPDLOG_INFO(
            "{}: {} millisecs ({} calls)",
            name,
            std::chrono::duration_cast<std::chrono::milliseconds>(measurement.duration).count(),
            measurement.call_count
    );
}
}  // namespace utils::profiling
