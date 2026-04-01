#include "ClpArchiveDecoder.hpp"

#include <cstdint>
#include <exception>
#include <memory>
#include <new>
#include <string>
#include <utility>

#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ArchiveReader.hpp>
#include <clp_s/Defs.hpp>

#include "ClpArchiveReader.hpp"
#include "SfaErrorCode.hpp"

namespace clp_s::ffi::sfa {
auto ClpArchiveDecoder::create(ClpArchiveReader& reader)
        -> ystdlib::error_handling::Result<ClpArchiveDecoder> {
    try {
        return ClpArchiveDecoder{reader.m_archive_reader->read_all_tables()};
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
    m_decode_completed = false;
}

auto ClpArchiveDecoder::move_from(ClpArchiveDecoder& rhs) noexcept -> void {
    m_tables = std::move(rhs.m_tables);
    m_log_events = std::move(rhs.m_log_events);
    m_decode_completed = std::exchange(rhs.m_decode_completed, false);
}

auto ClpArchiveDecoder::decode_all() -> ystdlib::error_handling::Result<void> {
    if (m_decode_completed) {
        return ystdlib::error_handling::success();
    }

    try {
        m_log_events.clear();

        for (auto const& table : m_tables) {
            std::string message;
            epochtime_t timestamp{0};
            int64_t log_event_idx{0};

            while (table->get_next_message_with_metadata(message, timestamp, log_event_idx)) {
                m_log_events.emplace_back(log_event_idx, timestamp, message);
            }
        }

        m_decode_completed = true;
        return ystdlib::error_handling::success();
    } catch (std::bad_alloc const&) {
        SPDLOG_ERROR("Failed to decode all log events in ClpArchiveDecoder: out of memory.");
        return SfaErrorCode{SfaErrorCodeEnum::NoMemory};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while decoding all log events in ClpArchiveDecoder: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::Failure};
    }
}
}  // namespace clp_s::ffi::sfa
