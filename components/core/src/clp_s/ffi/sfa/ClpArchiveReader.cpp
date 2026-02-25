#include "ClpArchiveReader.hpp"

#include <stdexcept>

namespace clp_s::ffi::sfa {
auto ClpArchiveReader::create(std::string_view archive_path) -> std::unique_ptr<ClpArchiveReader> {
    auto path = get_path_object_for_raw_path(archive_path);
    auto reader = std::make_unique<clp_s::ArchiveReader>();
    reader->open(path, NetworkAuthOption{});

    return std::unique_ptr<ClpArchiveReader>(new ClpArchiveReader(std::move(reader)));
}

ClpArchiveReader::~ClpArchiveReader() noexcept {
    if (nullptr != m_archive_reader) {
        try {
            close();
        } catch (...) {
            // Suppress exceptions in destructor.
        }
    }
}

auto ClpArchiveReader::get_archive_id() const -> std::string {
    if (nullptr == m_archive_reader) {
        throw std::logic_error("ClpArchiveReader is closed.");
    }

    return std::string{m_archive_reader->get_archive_id()};
}

void ClpArchiveReader::close() {
    if (nullptr == m_archive_reader) {
        return;
    }

    try {
        m_archive_reader->close();
    } catch (...) {
        m_archive_reader.reset();
        throw;
    }

    m_archive_reader.reset();
}
}  // namespace clp_s::ffi::sfa
