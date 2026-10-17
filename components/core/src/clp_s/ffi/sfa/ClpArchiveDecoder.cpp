#include "ClpArchiveDecoder.hpp"

#include <cstdint>
#include <exception>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <string>
#include <utility>

#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ArchiveReader.hpp>
#include <clp_s/Defs.hpp>
#include <clp_s/SchemaReader.hpp>

#include "ClpArchiveReader.hpp"
#include "LogEvent.hpp"
#include "SfaErrorCode.hpp"

namespace clp_s::ffi::sfa {
template <typename ReturnType>
using Result = ystdlib::error_handling::Result<ReturnType>;

auto ClpArchiveDecoder::create(ClpArchiveReader& reader) -> Result<ClpArchiveDecoder> {
    try {
        return ClpArchiveDecoder{
                reader.m_archive_reader->read_all_tables(),
                reader.m_archive_reader->has_log_order()
        };
    } catch (std::bad_alloc const&) {
        SPDLOG_ERROR("Failed to create ClpArchiveDecoder: out of memory.");
        return SfaErrorCode{SfaErrorCodeEnum::NoMemory};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while creating ClpArchiveDecoder: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::Failure};
    }
}

ClpArchiveDecoder::ClpArchiveDecoder(ClpArchiveDecoder&& rhs) noexcept {
    move_from(rhs);
}

auto ClpArchiveDecoder::operator=(ClpArchiveDecoder&& rhs) noexcept -> ClpArchiveDecoder& {
    if (this == &rhs) {
        return *this;
    }

    close();
    move_from(rhs);
    return *this;
}

ClpArchiveDecoder::~ClpArchiveDecoder() noexcept {
    close();
}

auto ClpArchiveDecoder::close() noexcept -> void {
    m_tables.clear();
    m_log_events.clear();
    m_has_log_order = false;
}

auto ClpArchiveDecoder::move_from(ClpArchiveDecoder& rhs) noexcept -> void {
    m_tables = std::move(rhs.m_tables);
    m_log_events = std::move(rhs.m_log_events);
    m_has_log_order = std::exchange(rhs.m_has_log_order, false);
}

auto ClpArchiveDecoder::get_next_log_event() -> Result<std::optional<LogEvent>> {
    try {
        auto const has_next_log_event
                = m_has_log_order ? decode_next_log_event_in_order() : decode_next_log_event();
        if (false == has_next_log_event) {
            return std::nullopt;
        }

        return m_log_events.back();
    } catch (std::bad_alloc const&) {
        SPDLOG_ERROR("Failed to decode log event in ClpArchiveDecoder: out of memory.");
        return SfaErrorCode{SfaErrorCodeEnum::NoMemory};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while decoding log event in ClpArchiveDecoder: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::Failure};
    }
}

auto ClpArchiveDecoder::collect_log_events() -> Result<std::span<LogEvent const>> {
    while (YSTDLIB_ERROR_HANDLING_TRYX(get_next_log_event()).has_value()) {}

    return std::span<LogEvent const>{m_log_events};
}

auto ClpArchiveDecoder::append_next_log_event(clp_s::SchemaReader& table) -> bool {
    std::string message;
    epochtime_t timestamp{0};
    int64_t log_event_idx{0};

    if (table.get_next_message_with_metadata(message, timestamp, log_event_idx)) {
        m_log_events.emplace_back(message, timestamp, log_event_idx);
        return true;
    }

    return false;
}

auto ClpArchiveDecoder::decode_next_log_event() -> bool {
    for (auto const& table : m_tables) {
        if (table->done()) {
            continue;
        }

        return append_next_log_event(*table);
    }

    return false;
}

auto ClpArchiveDecoder::decode_next_log_event_in_order() -> bool {
    std::shared_ptr<clp_s::SchemaReader> next_table;
    int64_t next_log_event_idx{0};
    bool found_next_table{false};

    for (auto const& table : m_tables) {
        if (table->done()) {
            continue;
        }

        auto const table_log_event_idx{table->get_next_log_event_idx()};
        if (false == found_next_table || table_log_event_idx < next_log_event_idx) {
            next_table = table;
            next_log_event_idx = table_log_event_idx;
            found_next_table = true;
        }
    }

    if (false == found_next_table) {
        return false;
    }

    return append_next_log_event(*next_table);
}
}  // namespace clp_s::ffi::sfa
