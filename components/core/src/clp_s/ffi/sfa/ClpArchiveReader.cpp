#include "ClpArchiveReader.hpp"

#include <cstdint>
#include <exception>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/BufferReader.hpp>
#include <clp_s/archive_constants.hpp>
#include <clp_s/ArchiveReader.hpp>
#include <clp_s/ffi/sfa/SfaErrorCode.hpp>
#include <clp_s/ffi/sfa/LogEvent.hpp>
#include <clp_s/InputConfig.hpp>

namespace clp_s::ffi::sfa {
template <typename ReturnType>
using Result = ystdlib::error_handling::Result<ReturnType>;

auto ClpArchiveReader::create(std::string_view archive_path) -> Result<ClpArchiveReader> {
    std::unique_ptr<clp_s::ArchiveReader> reader;

    try {
        auto path{get_path_object_for_raw_path(archive_path)};
        reader = std::make_unique<clp_s::ArchiveReader>();
        reader->open(path, NetworkAuthOption{});

        auto clp_archive_reader{ClpArchiveReader{std::move(reader), nullptr}};
        YSTDLIB_ERROR_HANDLING_TRYV(clp_archive_reader.precompute_archive_metadata());

        return clp_archive_reader;
    } catch (std::bad_alloc const&) {
        SPDLOG_ERROR(
                "Failed to create ClpArchiveReader for archive {}: out of memory.",
                archive_path
        );
        return SfaErrorCode{SfaErrorCodeEnum::NoMemory};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while creating ClpArchiveReader: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::IoFailure};
    }
}

auto ClpArchiveReader::create(std::vector<char>&& archive_data) -> Result<ClpArchiveReader> {
    // `clp_s::ArchiveReader` requires an archive ID, but `clp_s::ffi::sfa::ClpArchiveReader` never
    // uses it. Provide a dummy value solely to satisfy the constructor.
    constexpr std::string_view cDefaultArchiveId{"default"};

    std::unique_ptr<clp_s::ArchiveReader> archive_reader;
    std::shared_ptr<std::vector<char>> archive_data_owner;

    try {
        archive_data_owner = std::make_shared<std::vector<char>>(std::move(archive_data));
        auto reader{std::make_shared<clp::BufferReader>(
                archive_data_owner->data(),
                archive_data_owner->size()
        )};

        archive_reader = std::make_unique<clp_s::ArchiveReader>();
        archive_reader->open(reader, cDefaultArchiveId);

        auto clp_archive_reader{
                ClpArchiveReader{std::move(archive_reader), std::move(archive_data_owner)}
        };
        YSTDLIB_ERROR_HANDLING_TRYV(clp_archive_reader.precompute_archive_metadata());

        return clp_archive_reader;
    } catch (std::bad_alloc const&) {
        SPDLOG_ERROR("Failed to create ClpArchiveReader: out of memory.");
        return SfaErrorCode{SfaErrorCodeEnum::NoMemory};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while creating ClpArchiveReader: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::IoFailure};
    }
}

ClpArchiveReader::ClpArchiveReader(
        std::unique_ptr<clp_s::ArchiveReader> reader,
        std::shared_ptr<std::vector<char>> archive_data
)
        : m_archive_reader{std::move(reader)},
          m_archive_data{std::move(archive_data)} {}

ClpArchiveReader::ClpArchiveReader(ClpArchiveReader&& rhs) noexcept {
    move_from(rhs);
}

auto ClpArchiveReader::operator=(ClpArchiveReader&& rhs) noexcept -> ClpArchiveReader& {
    if (this == &rhs) {
        return *this;
    }

    close();
    move_from(rhs);
    return *this;
}

ClpArchiveReader::~ClpArchiveReader() noexcept {
    close();
}

auto ClpArchiveReader::close() noexcept -> void {
    // FFI frontends may invoke destruction paths multiple times (e.g., explicit close followed by
    // GC finalization). Guard against this by checking for a null reader before attempting to
    // close.
    if (nullptr != m_archive_reader) {
        try {
            m_archive_reader->close();
        } catch (std::exception const& ex) {
            SPDLOG_ERROR("Exception while closing ClpArchiveReader: {}", ex.what());
        }
        m_archive_reader.reset();
    }
    m_event_count = 0;
    m_file_names.clear();
    m_file_infos.clear();
    m_tables.clear();
    m_log_events.clear();
}

auto ClpArchiveReader::move_from(ClpArchiveReader& rhs) noexcept -> void {
    m_archive_reader = std::move(rhs.m_archive_reader);
    m_archive_data = std::move(rhs.m_archive_data);
    m_event_count = std::exchange(rhs.m_event_count, 0);
    m_file_names = std::move(rhs.m_file_names);
    m_file_infos = std::move(rhs.m_file_infos);
    m_tables = std::move(rhs.m_tables);
    m_log_events = std::move(rhs.m_log_events);
}

auto ClpArchiveReader::decode_all() -> Result<std::vector<LogEvent>> {
    if (m_log_events.size() == m_event_count) {
        return m_log_events;
    }

    if (nullptr == m_archive_reader) {
        return SfaErrorCode{SfaErrorCodeEnum::NotInit};
    }

    try {
        m_log_events.clear();
        m_log_events.reserve(m_event_count);

        std::string message;
        int64_t timestamp{0};
        int64_t log_event_idx{0};
        while (true) {
            std::shared_ptr<clp_s::SchemaReader> next_table{nullptr};
            int64_t next_idx{0};

            for (auto const& table : m_tables) {
                if (nullptr == table) {
                    SPDLOG_ERROR("Failed to decode archive: encountered null schema table.");
                    return SfaErrorCode{SfaErrorCodeEnum::IoFailure};
                }
                if (table->done()) {
                    continue;
                }
                auto const current_idx{table->get_next_log_event_idx()};
                if (nullptr == next_table || current_idx < next_idx) {
                    next_table = table;
                    next_idx = current_idx;
                }
            }

            if (nullptr == next_table) {
                break;
            }

            if (next_table->get_next_message_with_metadata(
                        message,
                        timestamp,
                        log_event_idx,
                        nullptr
                ))
            {
                m_log_events.emplace_back(log_event_idx, timestamp, std::move(message));
            }
        }
        return m_log_events;
    } catch (std::bad_alloc const&) {
        SPDLOG_ERROR("Failed to decode archive: out of memory.");
        return SfaErrorCode{SfaErrorCodeEnum::NoMemory};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while decoding archive: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::IoFailure};
    }
}

auto ClpArchiveReader::precompute_archive_metadata() -> Result<void> {
    auto const& range_index{m_archive_reader->get_range_index()};
    m_file_names.reserve(range_index.size());
    m_file_infos.reserve(range_index.size());

    int64_t prev_end_idx{0};
    for (auto const& range : range_index) {
        auto const start_idx{static_cast<int64_t>(range.start_index)};
        auto const end_idx{static_cast<int64_t>(range.end_index)};
        if (start_idx >= end_idx || start_idx != prev_end_idx) {
            return SfaErrorCode{SfaErrorCodeEnum::MalformedRangeIndex};
        }
        m_event_count += static_cast<uint64_t>(end_idx - start_idx);

        auto const filename_it{
                range.fields.find(std::string{clp_s::constants::range_index::cFilename})
        };
        if (range.fields.end() == filename_it || false == filename_it->is_string()) {
            return SfaErrorCode{SfaErrorCodeEnum::MalformedRangeIndex};
        }
        auto const filename{filename_it->get<std::string>()};

        m_file_names.push_back(filename);
        m_file_infos.emplace_back(filename, start_idx, end_idx);

        prev_end_idx = end_idx;
    }

    m_archive_reader->read_dictionaries_and_metadata();
    m_archive_reader->open_packed_streams();
    m_tables = m_archive_reader->read_all_tables();

    return ystdlib::error_handling::success();
}
}  // namespace clp_s::ffi::sfa
