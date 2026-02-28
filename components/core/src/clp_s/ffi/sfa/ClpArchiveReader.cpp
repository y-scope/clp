#include "ClpArchiveReader.hpp"

#include <stdexcept>
#include <system_error>

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

ClpArchiveReader::~ClpArchiveReader() noexcept {
    if (nullptr != m_archive_reader) {
        try {
            m_archive_reader->close();
        } catch (...) {
            // Suppress exceptions in destructor.
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
