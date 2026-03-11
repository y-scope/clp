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
        return ClpArchiveReader{std::move(reader), nullptr};
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
        return ClpArchiveReader{std::move(archive_reader), std::move(archive_data_owner)};
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
}

ClpArchiveReader::ClpArchiveReader(
        std::unique_ptr<clp_s::ArchiveReader> reader,
        std::shared_ptr<std::vector<char>> archive_data
)
        : m_archive_reader{std::move(reader)},
          m_archive_data{std::move(archive_data)} {
    precompute_archive_metadata();
}

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
}

auto ClpArchiveReader::move_from(ClpArchiveReader& rhs) noexcept -> void {
    m_archive_reader = std::move(rhs.m_archive_reader);
    m_archive_data = std::move(rhs.m_archive_data);
    m_event_count = std::exchange(rhs.m_event_count, 0);
}

auto ClpArchiveReader::precompute_archive_metadata() -> void {
    auto const& range_index{m_archive_reader->get_range_index()};

    for (auto const& range : range_index) {
        auto const start_idx{static_cast<uint64_t>(range.start_index)};
        auto const end_idx{static_cast<uint64_t>(range.end_index)};
        m_event_count += end_idx - start_idx;
    }
}

auto ClpArchiveReader::get_archive_id() const -> Result<std::string> {
    if (nullptr == m_archive_reader) {
        return SfaErrorCode{SfaErrorCodeEnum::NotInit};
    }

    return std::string{m_archive_reader->get_archive_id()};
}
}  // namespace clp_s::ffi::sfa
