#include "ArchiveMetadata.hpp"

#include <sys/stat.h>

#include <fmt/core.h>
#include <ystdlib/containers/Array.hpp>

#include "../FileReader.hpp"

namespace clp::streaming_archive {
ArchiveMetadata::ArchiveMetadata(
        archive_format_version_t archive_format_version,
        std::string creator_id,
        uint64_t creation_idx
)
        : m_archive_format_version(archive_format_version),
          m_creator_id(std::move(creator_id)),
          m_creation_idx(creation_idx) {}

auto ArchiveMetadata::create_from_file(std::string_view file_path) -> ArchiveMetadata {
    FileReader file_reader{std::string(file_path)};
    struct stat file_stat{};
    if (auto const clp_rc = file_reader.try_fstat(file_stat);
        clp::ErrorCode::ErrorCode_Success != clp_rc)
    {
        throw OperationFailed(clp_rc, __FILENAME__, __LINE__);
    }

    ystdlib::containers::Array<char> buf(static_cast<size_t>(file_stat.st_size));
    if (auto const clp_rc = file_reader.try_read_exact_length(buf.data(), buf.size());
        clp::ErrorCode::ErrorCode_Success != clp_rc)
    {
        throw OperationFailed(clp_rc, __FILENAME__, __LINE__);
    }

    ArchiveMetadata metadata;
    msgpack::object_handle const obj_handle = msgpack::unpack(buf.data(), buf.size());
    msgpack::object const obj = obj_handle.get();
    obj.convert(metadata);
    return metadata;
}

void ArchiveMetadata::expand_time_range(epochtime_t begin_timestamp, epochtime_t end_timestamp) {
    if (begin_timestamp < m_begin_timestamp) {
        m_begin_timestamp = begin_timestamp;
    }
    if (end_timestamp > m_end_timestamp) {
        m_end_timestamp = end_timestamp;
    }
}

void ArchiveMetadata::write_to_file(FileWriter& file_writer) const {
    std::ostringstream buf;
    msgpack::pack(buf, *this);
    auto const& string_buf = buf.str();
    file_writer.write(string_buf.data(), string_buf.size());
}
}  // namespace clp::streaming_archive
