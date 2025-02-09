#include "ArchiveReaderAdaptor.hpp"

#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <msgpack.hpp>
#include <spdlog/spdlog.h>

#include "../clp/BoundedReader.hpp"
#include "../clp/FileReader.hpp"
#include "archive_constants.hpp"
#include "InputConfig.hpp"
#include "ReaderUtils.hpp"
#include "SingleFileArchiveDefs.hpp"

namespace clp_s {

ArchiveReaderAdaptor::ArchiveReaderAdaptor(
        Path const& archive_path,
        NetworkAuthOption const& network_auth
)
        : m_archive_path{archive_path},
          m_network_auth{network_auth},
          m_timestamp_dictionary{std::make_shared<TimestampDictionaryReader>()},
          m_single_file_archive{false} {
    if (InputSource::Filesystem != archive_path.source
        || std::filesystem::is_regular_file(archive_path.path))
    {
        m_single_file_archive = true;
    }
}

ErrorCode
ArchiveReaderAdaptor::try_read_archive_file_info(ZstdDecompressor& decompressor, size_t size) {
    std::vector<char> buffer(size);
    auto rc = decompressor.try_read_exact_length(buffer.data(), size);
    if (ErrorCodeSuccess != rc) {
        return rc;
    }

    try {
        auto obj_handle = msgpack::unpack(buffer.data(), buffer.size());
        auto obj = obj_handle.get();
        // m_archive_file_info = obj.as<clp_s::ArchiveFileInfoPacket>();
        //  FIXME: the above should work, but does not. Hacking around it as below for now.
        if (obj.is_nil() || msgpack::type::MAP != obj.type) {
            return ErrorCodeCorrupt;
        }
        if (nullptr == obj.via.map.ptr) {
            return ErrorCodeCorrupt;
        }
        auto val = obj.via.map.ptr->val;
        if (val.is_nil() || msgpack::type::ARRAY != val.type) {
            return ErrorCodeCorrupt;
        }
        if (nullptr == val.via.array.ptr) {
            return ErrorCodeCorrupt;
        }
        auto arr = val.via.array;
        for (size_t i = 0; i < arr.size; ++i) {
            auto array_element = arr.ptr[i].as<clp_s::ArchiveFileInfo>();
            m_archive_file_info.files.push_back(array_element);
        }
        return ErrorCodeSuccess;
    } catch (std::exception const& e) {
        return ErrorCodeCorrupt;
    }
}

ErrorCode
ArchiveReaderAdaptor::try_read_timestamp_dictionary(ZstdDecompressor& decompressor, size_t size) {
    return m_timestamp_dictionary->read(decompressor);
}

ErrorCode ArchiveReaderAdaptor::try_read_archive_info(ZstdDecompressor& decompressor, size_t size) {
    std::vector<char> buffer(size);
    auto rc = decompressor.try_read_exact_length(buffer.data(), buffer.size());
    if (ErrorCodeSuccess != rc) {
        return rc;
    }

    try {
        auto obj_handle = msgpack::unpack(buffer.data(), buffer.size());
        auto obj = obj_handle.get();
        m_archive_info = obj.as<ArchiveInfoPacket>();
    } catch (std::exception const& e) {
        return ErrorCodeCorrupt;
    }

    if (1 != m_archive_info.num_segments) {
        return ErrorCodeUnsupported;
    }
    return ErrorCodeSuccess;
}

ErrorCode ArchiveReaderAdaptor::load_archive_metadata() {
    constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024;
    m_reader = try_create_reader_at_header();
    if (nullptr == m_reader) {
        return ErrorCodeFileNotFound;
    }

    if (auto const rc = try_read_header(*m_reader); ErrorCodeSuccess != rc) {
        return rc;
    }

    m_files_section_offset = sizeof(m_archive_header) + m_archive_header.metadata_section_size;
    clp::BoundedReader bounded_reader{m_reader.get(), m_files_section_offset};
    ZstdDecompressor decompressor;
    decompressor.open(bounded_reader, cDecompressorFileReadBufferCapacity);
    auto const rc = try_read_archive_metadata(decompressor);
    decompressor.close();
    return rc;
}

ErrorCode ArchiveReaderAdaptor::try_read_header(clp::ReaderInterface& reader) {
    auto const clp_rc = reader.try_read_exact_length(
            reinterpret_cast<char*>(&m_archive_header),
            sizeof(m_archive_header)
    );
    if (clp::ErrorCode::ErrorCode_Success != clp_rc) {
        return ErrorCodeErrno;
    }

    if (0
        != std::memcmp(
                m_archive_header.magic_number,
                cStructuredSFAMagicNumber,
                sizeof(cStructuredSFAMagicNumber)
        ))
    {
        return ErrorCodeMetadataCorrupted;
    }

    switch (static_cast<ArchiveCompressionType>(m_archive_header.compression_type)) {
        case ArchiveCompressionType::Zstd:
            break;
        default:
            return ErrorCodeUnsupported;
    }
    return ErrorCodeSuccess;
}

ErrorCode ArchiveReaderAdaptor::try_read_archive_metadata(ZstdDecompressor& decompressor) {
    uint8_t num_metadata_packets{};
    auto rc = decompressor.try_read_numeric_value(num_metadata_packets);
    if (ErrorCodeSuccess != rc) {
        return rc;
    }

    for (size_t i = 0; i < num_metadata_packets; ++i) {
        ArchiveMetadataPacketType packet_type;
        uint32_t packet_size;
        rc = decompressor.try_read_numeric_value(packet_type);
        if (ErrorCodeSuccess != rc) {
            return rc;
        }
        rc = decompressor.try_read_numeric_value(packet_size);
        if (ErrorCodeSuccess != rc) {
            return rc;
        }

        switch (packet_type) {
            case ArchiveMetadataPacketType::ArchiveFileInfo:
                rc = try_read_archive_file_info(decompressor, packet_size);
                break;
            case ArchiveMetadataPacketType::TimestampDictionary:
                rc = try_read_timestamp_dictionary(decompressor, packet_size);
                break;
            case ArchiveMetadataPacketType::ArchiveInfo:
                rc = try_read_archive_info(decompressor, packet_size);
                break;
            default:
                break;
        }
        if (ErrorCodeSuccess != rc) {
            return rc;
        }
    }
    return ErrorCodeSuccess;
}

std::shared_ptr<clp::ReaderInterface> ArchiveReaderAdaptor::try_create_reader_at_header() {
    if (InputSource::Filesystem == m_archive_path.source && false == m_single_file_archive) {
        try {
            return std::make_shared<clp::FileReader>(
                    m_archive_path.path + constants::cArchiveHeaderFile
            );
        } catch (std::exception const& e) {
            SPDLOG_ERROR("Failed to open archive header for reading - {}", e.what());
            return nullptr;
        }
    } else {
        return ReaderUtils::try_create_reader(m_archive_path, m_network_auth);
    }
}

std::unique_ptr<clp::ReaderInterface> ArchiveReaderAdaptor::checkout_reader_for_section(
        std::string_view section
) {
    if (m_current_reader_holder.has_value()) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }

    m_current_reader_holder.emplace(section);
    if (m_single_file_archive) {
        return checkout_reader_for_sfa_section(section);
    } else {
        return std::make_unique<clp::FileReader>(m_archive_path.path + std::string{section});
    }
}

std::unique_ptr<clp::ReaderInterface> ArchiveReaderAdaptor::checkout_reader_for_sfa_section(
        std::string_view section
) {
    auto it = std::find_if(
            m_archive_file_info.files.begin(),
            m_archive_file_info.files.end(),
            [&](ArchiveFileInfo& info) { return info.n == section; }
    );
    if (m_archive_file_info.files.end() == it) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }

    size_t curr_pos{};
    if (auto rc = m_reader->try_get_pos(curr_pos); clp::ErrorCode::ErrorCode_Success != rc) {
        throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
    }

    size_t file_offset = m_files_section_offset + it->o;
    ++it;
    size_t next_file_offset{m_archive_header.compressed_size};
    if (m_archive_file_info.files.end() != it) {
        next_file_offset = m_files_section_offset + it->o;
    }

    if (curr_pos > file_offset) {
        throw OperationFailed(ErrorCodeCorrupt, __FILENAME__, __LINE__);
    }

    if (curr_pos != file_offset) {
        if (auto rc = m_reader->try_seek_from_begin(file_offset);
            clp::ErrorCode::ErrorCode_Success != rc)
        {
            throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
        }
    }

    return std::make_unique<clp::BoundedReader>(m_reader.get(), next_file_offset);
}

void ArchiveReaderAdaptor::checkin_reader_for_section(std::string_view section) {
    if (false == m_current_reader_holder.has_value()) {
        throw OperationFailed(ErrorCodeNotInit, __FILENAME__, __LINE__);
    }

    if (m_current_reader_holder.value() != section) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }

    m_current_reader_holder.reset();
}
}  // namespace clp_s
