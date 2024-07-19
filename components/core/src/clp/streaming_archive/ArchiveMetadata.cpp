#include "ArchiveMetadata.hpp"

namespace clp::streaming_archive {
ArchiveMetadata::ArchiveMetadata(
        archive_format_version_t archive_format_version,
        std::string creator_id,
        uint64_t creation_idx
)
        : m_archive_format_version(archive_format_version),
          m_creator_id(std::move(creator_id)),
          m_creation_idx(creation_idx) {
    if (m_creator_id.length() > UINT16_MAX) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    m_creator_id_len = m_creator_id.length();

    // NOTE: We set this to the size of this metadata on disk; when adding new members that will be
    // written to disk, you must update this
    m_compressed_size += sizeof(m_archive_format_version) + sizeof(m_creator_id_len)
                         + m_creator_id.length() + sizeof(m_creation_idx)
                         + sizeof(m_uncompressed_size) + sizeof(m_begin_timestamp)
                         + sizeof(m_end_timestamp) + sizeof(m_compressed_size);
}

ArchiveMetadata::ArchiveMetadata(FileReader& file_reader) {
    file_reader.read_numeric_value(m_archive_format_version, false);
    file_reader.read_numeric_value(m_creator_id_len, false);
    file_reader.read_string(m_creator_id_len, m_creator_id, false);
    file_reader.read_numeric_value(m_creation_idx, false);
    file_reader.read_numeric_value(m_uncompressed_size, false);
    file_reader.read_numeric_value(m_compressed_size, false);
    file_reader.read_numeric_value(m_begin_timestamp, false);
    file_reader.read_numeric_value(m_end_timestamp, false);
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
    file_writer.write_numeric_value(m_archive_format_version);
    file_writer.write_numeric_value(m_creator_id_len);
    file_writer.write_string(m_creator_id);
    file_writer.write_numeric_value(m_creation_idx);
    file_writer.write_numeric_value(m_uncompressed_size + m_dynamic_uncompressed_size);
    file_writer.write_numeric_value(m_compressed_size + m_dynamic_uncompressed_size);
    file_writer.write_numeric_value(m_begin_timestamp);
    file_writer.write_numeric_value(m_end_timestamp);
}
}  // namespace clp::streaming_archive
