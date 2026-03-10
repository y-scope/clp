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

auto ClpArchiveReader::create(std::string_view archive_path) -> Result<ClpArchiveReader> {
    std::unique_ptr<clp_s::ArchiveReader> reader;

    try {
        auto path{get_path_object_for_raw_path(archive_path)};
        reader = std::make_unique<clp_s::ArchiveReader>();
        reader->open(path, NetworkAuthOption{});
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

    return ClpArchiveReader{std::move(reader), nullptr};
}

auto ClpArchiveReader::create(std::span<char const> archive_data, std::string_view archive_id)
        -> Result<ClpArchiveReader> {
    std::unique_ptr<clp_s::ArchiveReader> archive_reader;
    std::shared_ptr<std::vector<char>> archive_data_owner;

    try {
        archive_data_owner
                = std::make_shared<std::vector<char>>(archive_data.begin(), archive_data.end());
        auto reader{std::make_shared<clp::BufferReader>(
                archive_data_owner->data(),
                archive_data_owner->size()
        )};

        archive_reader = std::make_unique<clp_s::ArchiveReader>();
        archive_reader->open(reader, archive_id);
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

    return ClpArchiveReader{std::move(archive_reader), std::move(archive_data_owner)};
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

    uint64_t event_count{0};
    for (auto const& range : m_archive_reader->get_range_index()) {
        event_count += static_cast<uint64_t>(range.end_index - range.start_index);
    }

    return event_count;
}
}  // namespace clp_s::ffi::sfa
