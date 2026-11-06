#include "ArchiveReaderAdaptor.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <msgpack.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <clp/BufferReader.hpp>

#include "../clp/BoundedReader.hpp"
#include "../clp/ErrorCode.hpp"
#include "../clp/FileReader.hpp"
#include "archive_constants.hpp"
#include "ErrorCode.hpp"
#include "InputConfig.hpp"
#include "RangeIndexWriter.hpp"
#include "ReaderUtils.hpp"
#include "SingleFileArchiveDefs.hpp"

namespace clp_s {
ArchiveReaderAdaptor::ArchiveReaderAdaptor(
        Path const& archive_path,
        NetworkAuthOption const& network_auth
)
        : m_archive_path{archive_path},
          m_network_auth{network_auth},
          m_timestamp_dictionary{std::make_shared<TimestampDictionaryReader>()} {
    if (InputSource::Filesystem != archive_path.source
        || std::filesystem::is_regular_file(archive_path.path))
    {
        m_single_file_archive = true;
    }
}

ArchiveReaderAdaptor::ArchiveReaderAdaptor(
        std::shared_ptr<clp::ReaderInterface> single_file_archive_reader
)
        : m_single_file_archive{true},
          m_timestamp_dictionary{std::make_shared<TimestampDictionaryReader>()},
          m_reader{std::move(single_file_archive_reader)} {
    if (nullptr == m_reader) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }

    if (auto const rc = m_reader->try_seek_from_begin(0); clp::ErrorCode::ErrorCode_Success != rc) {
        throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
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
    return m_timestamp_dictionary->read(
            decompressor,
            m_archive_header.has_deprecated_timestamp_format()
    );
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

auto ArchiveReaderAdaptor::try_read_range_index(ZstdDecompressor& decompressor, size_t size)
        -> ErrorCode {
    std::vector<char> buffer(size);
    if (auto const rc = decompressor.try_read_exact_length(buffer.data(), buffer.size());
        ErrorCodeSuccess != rc)
    {
        return rc;
    }

    auto range_index_json = nlohmann::json::from_msgpack(buffer.begin(), buffer.end(), true, false);
    if (false == range_index_json.is_array()) {
        return ErrorCodeCorrupt;
    }

    for (auto& range_index_entry : range_index_json) {
        if (false == range_index_entry.contains(RangeIndexWriter::cStartIndexName)
            || false == range_index_entry.at(RangeIndexWriter::cStartIndexName).is_number_integer())
        {
            return ErrorCodeCorrupt;
        }
        if (false == range_index_entry.contains(RangeIndexWriter::cEndIndexName)
            || false == range_index_entry.at(RangeIndexWriter::cEndIndexName).is_number_integer())
        {
            return ErrorCodeCorrupt;
        }
        if (false == range_index_entry.contains(RangeIndexWriter::cMetadataFieldsName)
            || false == range_index_entry.at(RangeIndexWriter::cMetadataFieldsName).is_object())
        {
            return ErrorCodeCorrupt;
        }
        size_t start_index{};
        size_t end_index{};
        try {
            start_index = range_index_entry.at(RangeIndexWriter::cStartIndexName)
                                  .template get<size_t>();
            end_index
                    = range_index_entry.at(RangeIndexWriter::cEndIndexName).template get<size_t>();
        } catch (std::exception const&) {
            return ErrorCodeCorrupt;
        }
        if (start_index > end_index) {
            return ErrorCodeCorrupt;
        }
        m_range_index.emplace_back(
                start_index,
                end_index,
                std::move(range_index_entry.at(RangeIndexWriter::cMetadataFieldsName))
        );
        if (start_index != end_index) {
            m_non_empty_range_metadata_map.emplace(end_index, m_range_index.back().fields);
        }
    }
    return ErrorCodeSuccess;
}

auto
ArchiveReaderAdaptor::try_read_unknown_metadata_packet(ZstdDecompressor& decompressor, size_t size)
        -> ErrorCode {
    std::vector<char> buffer(size);
    return decompressor.try_read_exact_length(buffer.data(), buffer.size());
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
                cStructuredSFAMagicNumber.data(),
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
            case ArchiveMetadataPacketType::RangeIndex:
                rc = try_read_range_index(decompressor, packet_size);
                break;
            default:
                rc = try_read_unknown_metadata_packet(decompressor, packet_size);
                break;
        }
        if (ErrorCodeSuccess != rc) {
            return rc;
        }
    }
    return ErrorCodeSuccess;
}

std::shared_ptr<clp::ReaderInterface> ArchiveReaderAdaptor::try_create_reader_at_header() {
    if (nullptr != m_reader) {
        return m_reader;
    }

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
        return try_create_reader(m_archive_path, m_network_auth);
    }
}

std::unique_ptr<clp::ReaderInterface> ArchiveReaderAdaptor::checkout_reader_for_section(
        std::string_view section
) {
    if (false == m_single_file_archive) {
        return std::make_unique<clp::FileReader>(m_archive_path.path + std::string{section});
    }
    if (auto const it{m_buffered_sections.find(std::string{section})};
        m_buffered_sections.end() != it)
    {
        return std::make_unique<clp::BufferReader>(it->second.data(), it->second.size());
    }
    return checkout_reader_for_sfa_section(section);
}

std::unique_ptr<clp::ReaderInterface> ArchiveReaderAdaptor::checkout_reader_for_sfa_section(
        std::string_view section
) {
    if (m_current_reader_holder.has_value()) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }

    auto const section_it{std::find_if(
            m_archive_file_info.files.cbegin(),
            m_archive_file_info.files.cend(),
            [&](ArchiveFileInfo const& info) { return info.n == section; }
    )};
    if (m_archive_file_info.files.cend() == section_it) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }

    size_t curr_pos{};
    if (auto rc = m_reader->try_get_pos(curr_pos); clp::ErrorCode::ErrorCode_Success != rc) {
        throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
    }
    auto const [begin_offset, end_offset]{get_sfa_section_bounds(section_it)};
    if (curr_pos > begin_offset) {
        SPDLOG_ERROR(
                "Single-file archive sections must be read in on-disk order; cannot read '{}' at "
                "offset {} when the reader is already at offset {}.",
                section,
                begin_offset,
                curr_pos
        );
        throw OperationFailed(ErrorCodeCorrupt, __FILENAME__, __LINE__);
    }

    for (auto skipped_it{m_archive_file_info.files.cbegin()}; skipped_it != section_it;
         ++skipped_it)
    {
        auto const [skipped_begin_offset, skipped_end_offset]{get_sfa_section_bounds(skipped_it)};
        if (skipped_begin_offset < curr_pos || m_buffered_sections.contains(skipped_it->n)) {
            continue;
        }
        seek_to(skipped_begin_offset);
        std::vector<char> buffer(skipped_end_offset - skipped_begin_offset);
        if (auto const rc{m_reader->try_read_exact_length(buffer.data(), buffer.size())};
            clp::ErrorCode::ErrorCode_Success != rc)
        {
            throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
        }
        m_buffered_sections.emplace(skipped_it->n, std::move(buffer));
    }

    seek_to(begin_offset);
    m_current_reader_holder.emplace(section);
    return std::make_unique<clp::BoundedReader>(m_reader.get(), end_offset);
}

auto ArchiveReaderAdaptor::get_sfa_section_bounds(
        std::vector<ArchiveFileInfo>::const_iterator section_it
) const -> std::pair<size_t, size_t> {
    auto const archive_end_offset_result{
            ReaderUtils::try_uint64_to_size_t(m_archive_header.compressed_size)
    };
    if (archive_end_offset_result.has_error()) {
        throw OperationFailed(ErrorCodeOutOfBounds, __FILENAME__, __LINE__);
    }
    auto const archive_end_offset{archive_end_offset_result.value()};
    if (m_files_section_offset > archive_end_offset) {
        throw OperationFailed(ErrorCodeCorrupt, __FILENAME__, __LINE__);
    }
    auto const files_size{archive_end_offset - m_files_section_offset};
    auto const next_it{std::next(section_it)};
    auto const relative_end{m_archive_file_info.files.cend() != next_it ? next_it->o : files_size};
    if (section_it->o > relative_end || relative_end > files_size) {
        throw OperationFailed(ErrorCodeCorrupt, __FILENAME__, __LINE__);
    }
    return {m_files_section_offset + static_cast<size_t>(section_it->o),
            m_files_section_offset + static_cast<size_t>(relative_end)};
}

auto ArchiveReaderAdaptor::seek_to(size_t offset) -> void {
    size_t curr_pos{};
    if (auto rc = m_reader->try_get_pos(curr_pos); clp::ErrorCode::ErrorCode_Success != rc) {
        throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
    }
    if (curr_pos == offset) {
        return;
    }
    if (auto rc = m_reader->try_seek_from_begin(offset); clp::ErrorCode::ErrorCode_Success != rc) {
        throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
    }
}

void ArchiveReaderAdaptor::checkin_reader_for_section(std::string_view section) {
    if (false == m_single_file_archive || m_buffered_sections.contains(std::string{section})) {
        return;
    }

    if (false == m_current_reader_holder.has_value()) {
        throw OperationFailed(ErrorCodeNotInit, __FILENAME__, __LINE__);
    }

    if (m_current_reader_holder.value() != section) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }

    m_current_reader_holder.reset();
}

auto ArchiveReaderAdaptor::has_section(std::string_view section) const -> bool {
    return m_archive_file_info.files.end()
           != std::find_if(
                   m_archive_file_info.files.begin(),
                   m_archive_file_info.files.end(),
                   [&](ArchiveFileInfo const& info) { return info.n == section; }
           );
}

auto ArchiveReaderAdaptor::get_sections() const -> std::vector<std::string_view> {
    std::vector<std::string_view> sections;
    sections.reserve(m_archive_file_info.files.size());
    for (auto const& info : m_archive_file_info.files) {
        sections.emplace_back(info.n);
    }
    return sections;
}

auto ArchiveReaderAdaptor::get_metadata_for_log_event(int64_t log_event_idx)
        -> nlohmann::json const& {
    auto const it{m_non_empty_range_metadata_map.upper_bound(log_event_idx)};
    if (m_non_empty_range_metadata_map.end() == it || log_event_idx < 0) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }
    return it->second;
}
}  // namespace clp_s
