#ifndef TESTS_LOGSUPPRESSOR_HPP
#define TESTS_LOGSUPPRESSOR_HPP

#include <spdlog/spdlog.h>

/**
 * A class that suppresses logs so long as it's instantiated.
 */
class LogSuppressor {
public:
    LogSuppressor() {
        m_previous_logging_level = spdlog::default_logger()->level();
        spdlog::default_logger()->set_level(spdlog::level::off);
    }

    LogSuppressor(LogSuppressor const& other) = default;
    LogSuppressor(LogSuppressor&& other) noexcept = default;

    LogSuppressor& operator=(LogSuppressor const& other) = default;
    LogSuppressor& operator=(LogSuppressor&& other) noexcept = default;

    ~LogSuppressor() { spdlog::default_logger()->set_level(m_previous_logging_level); }

private:
    spdlog::level::level_enum m_previous_logging_level;
};

#endif  // TESTS_LOGSUPPRESSOR_HPP
