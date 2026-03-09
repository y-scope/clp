#include "ClpArchiveReader.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

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
    try {
        auto path = get_path_object_for_raw_path(archive_path);
        auto reader = std::make_unique<clp_s::ArchiveReader>();
        reader->open(path, NetworkAuthOption{});

        return ClpArchiveReader{std::move(reader)};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while creating ClpArchiveReader: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::IoFailure};
    }
}

auto ClpArchiveReader::create(std::span<char const> archive_data, std::string_view archive_id)
        -> Result<ClpArchiveReader> {
    try {
        auto archive_reader = std::make_unique<clp_s::ArchiveReader>();
        auto reader = std::make_shared<clp::BufferReader>(archive_data.data(), archive_data.size());
        archive_reader->open(reader, archive_id);

        return ClpArchiveReader{std::move(archive_reader)};
    } catch (std::exception const& ex) {
        SPDLOG_ERROR("Exception while creating ClpArchiveReader: {}", ex.what());
        return SfaErrorCode{SfaErrorCodeEnum::IoFailure};
    }
}

ClpArchiveReader::~ClpArchiveReader() {
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
