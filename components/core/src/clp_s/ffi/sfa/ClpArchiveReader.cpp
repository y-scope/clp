#include "ClpArchiveReader.hpp"

#include <cstdint>
#include <exception>
#include <memory>
#include <new>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/BufferReader.hpp>
#include <clp_s/ArchiveReader.hpp>
#include <clp_s/ffi/sfa/SfaErrorCode.hpp>
#include <clp_s/InputConfig.hpp>

namespace clp_s::ffi::sfa {
template <typename ReturnType>
using Result = ystdlib::error_handling::Result<ReturnType>;

namespace {
/**
 * Computes the total number of log events in the archive by summing event counts across all
 * range-index entries.
 *
 * @param archive_reader Open archive reader whose range index will be summed.
 * @return Total number of log events in the archive.
 */
[[nodiscard]] auto calculate_event_count(clp_s::ArchiveReader const& archive_reader) -> uint64_t {
    uint64_t event_count{0};
    for (auto const& range : archive_reader.get_range_index()) {
        event_count += static_cast<uint64_t>(range.end_index - range.start_index);
    }
    return event_count;
}
}  // namespace

// Default move constructor
ClpArchiveReader::ClpArchiveReader(ClpArchiveReader&&) noexcept = default;

// Default move assignment operator
auto ClpArchiveReader::operator=(ClpArchiveReader&&) noexcept -> ClpArchiveReader& = default;

auto ClpArchiveReader::create(std::string_view archive_path) -> Result<ClpArchiveReader> {
    std::unique_ptr<clp_s::ArchiveReader> reader;
    uint64_t event_count{0};

    try {
        auto path{get_path_object_for_raw_path(archive_path)};
        reader = std::make_unique<clp_s::ArchiveReader>();
        reader->open(path, NetworkAuthOption{});
        event_count = calculate_event_count(*reader);
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

    return ClpArchiveReader{std::move(reader), nullptr, event_count};
}

auto ClpArchiveReader::create(std::span<char const> archive_data, std::string_view archive_id)
        -> Result<ClpArchiveReader> {
    std::unique_ptr<clp_s::ArchiveReader> archive_reader;
    std::shared_ptr<std::vector<char>> archive_data_owner;
    uint64_t event_count{0};

    try {
        archive_data_owner
                = std::make_shared<std::vector<char>>(archive_data.begin(), archive_data.end());
        auto reader{std::make_shared<clp::BufferReader>(
                archive_data_owner->data(),
                archive_data_owner->size()
        )};

        archive_reader = std::make_unique<clp_s::ArchiveReader>();
        archive_reader->open(reader, archive_id);
        event_count = calculate_event_count(*archive_reader);
    } catch (std::bad_alloc const&) {
        SPDLOG_ERROR(
                "Failed to create ClpArchiveReader for archive {}: out of memory.",
                archive_id
        );
        return SfaErrorCode{SfaErrorCodeEnum::NoMemory};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while creating ClpArchiveReader: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::IoFailure};
    }

    return ClpArchiveReader{std::move(archive_reader), std::move(archive_data_owner), event_count};
}

ClpArchiveReader::~ClpArchiveReader() {
    // FFI frontends may invoke destruction paths multiple times (e.g., explicit close followed by
    // GC finalization). Guard against this by checking for a null reader before attempting to
    // close.
    if (nullptr == m_archive_reader) {
        return;
    }

    try {
        m_archive_reader->close();
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while closing ClpArchiveReader: {}", ex.what());
    }
    m_archive_reader.reset();
}

auto ClpArchiveReader::get_archive_id() const -> Result<std::string> {
    if (nullptr == m_archive_reader) {
        return SfaErrorCode{SfaErrorCodeEnum::NotInit};
    }

    return std::string{m_archive_reader->get_archive_id()};
}

auto ClpArchiveReader::get_event_count() const -> Result<uint64_t> {
    if (nullptr == m_archive_reader) {
        return SfaErrorCode{SfaErrorCodeEnum::NotInit};
    }

    return m_event_count;
}
}  // namespace clp_s::ffi::sfa
