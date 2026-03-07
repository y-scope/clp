#include "ClpArchiveReader.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/BufferReader.hpp>
#include <clp_s/ArchiveReader.hpp>
#include <clp_s/InputConfig.hpp>

namespace clp_s::ffi::sfa {
auto ClpArchiveReader::create(std::string_view archive_path)
        -> ystdlib::error_handling::Result<ClpArchiveReader> {
    try {
        auto path = get_path_object_for_raw_path(archive_path);
        auto reader = std::make_unique<clp_s::ArchiveReader>();
        reader->open(path, NetworkAuthOption{});

        return ClpArchiveReader{std::move(reader)};
    } catch (...) {
        return std::errc::io_error;
    }
}

auto ClpArchiveReader::create(std::span<char const> archive_data, std::string_view archive_id)
        -> ystdlib::error_handling::Result<ClpArchiveReader> {
    try {
        auto archive_reader = std::make_unique<clp_s::ArchiveReader>();
        auto reader = std::make_shared<clp::BufferReader>(archive_data.data(), archive_data.size());
        archive_reader->open(reader, archive_id);

        return ClpArchiveReader{std::move(archive_reader)};
    } catch (...) {
        return std::errc::io_error;
    }
}

ClpArchiveReader::~ClpArchiveReader() noexcept {
    if (nullptr != m_archive_reader) {
        try {
            m_archive_reader->close();
        } catch (std::exception const& ex) {
            SPDLOG_DEBUG("Ignoring exception while closing ClpArchiveReader: {}", ex.what());
        } catch (...) {
            SPDLOG_DEBUG("Ignoring unknown exception while closing ClpArchiveReader.");
        }
        m_archive_reader.reset();
    }
}

auto ClpArchiveReader::get_archive_id() const -> std::string {
    if (nullptr == m_archive_reader) {
        throw std::logic_error("ClpArchiveReader is closed.");
    }

    return std::string{m_archive_reader->get_archive_id()};
}

auto ClpArchiveReader::get_event_count() const -> uint64_t {
    if (nullptr == m_archive_reader) {
        throw std::logic_error("ClpArchiveReader is closed.");
    }

    uint64_t event_count{0};
    for (auto const& range : m_archive_reader->get_range_index()) {
        event_count += static_cast<uint64_t>(range.end_index - range.start_index);
    }

    return event_count;
}
}  // namespace clp_s::ffi::sfa
