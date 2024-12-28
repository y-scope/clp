#include "writer.hpp"

#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>

#include <fmt/core.h>
#include <msgpack.hpp>

#include "../../ErrorCode.hpp"
#include "../../ffi/encoding_methods.hpp"
#include "../../TraceableException.hpp"
#include "../ArchiveMetadata.hpp"
#include "../Constants.hpp"
#include "Defs.hpp"

namespace clp::streaming_archive::single_file_archive {

namespace {

constexpr size_t cReadBlockSize = 4096;

/**
 * Gets the size of a file specified by `file_path` and adds it to `offset`.
 * @param file_path
 * @param offset The current offset of the single file archive.
 * @throws OperationFailed if error getting file size.
 */
void update_offset(std::filesystem::path const& file_path, uint64_t& offset);

/**
 * Reads the content of a file and writes it to the archive.
 * @param file_path Path to the file to be read.
 * @param archive_writer Writer to write the file content to the archive.
 * @throws OperationFailed if reading the file fails.
 */
void write_archive_file(std::string const& file_path, FileWriter& archive_writer);

void update_offset(std::filesystem::path const& file_path, uint64_t& offset) {
    try {
        auto size = std::filesystem::file_size(file_path);
        offset += size;
    } catch (std::filesystem::filesystem_error const& e) {
        throw OperationFailed(
                ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                fmt::format("Failed to get file size: {}", e.what())
        );
    }
}

auto write_archive_file(std::filesystem::path const& file_path, FileWriter& archive_writer)
        -> void {
    FileReader reader(file_path.string());
    std::array<char, cReadBlockSize> read_buffer{};
    while (true) {
        size_t num_bytes_read{0};
        ErrorCode const error_code
                = reader.try_read(read_buffer.data(), cReadBlockSize, num_bytes_read);
        if (ErrorCode_EndOfFile == error_code) {
            break;
        }
        if (ErrorCode_Success != error_code) {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }
        archive_writer.write(read_buffer.data(), num_bytes_read);
    }
}
}  // namespace

auto get_segment_ids(segment_id_t last_segment_id) -> std::vector<std::string> {
    std::vector<std::string> segment_ids;

    if (last_segment_id < 0) {
        return segment_ids;
    }

    for (size_t i = 0; i <= last_segment_id; ++i) {
        segment_ids.emplace_back(std::to_string(i));
    }

    return segment_ids;
}

auto get_file_infos(
        std::filesystem::path const& multi_file_archive_path,
        std::vector<std::string> const& segment_ids
) -> std::vector<FileInfo> {
    std::vector<FileInfo> files;
    uint64_t offset = 0;

    for (auto const& static_archive_file_name : static_archive_file_names) {
        files.emplace_back(static_archive_file_name, offset);
        update_offset(multi_file_archive_path / static_archive_file_name, offset);
    }

    std::filesystem::path segment_dir_path = multi_file_archive_path / cSegmentsDirname;
    for (auto const& segment_id : segment_ids) {
        files.emplace_back(segment_id, offset);
        update_offset(segment_dir_path / segment_id, offset);
    }

    // Add sentinel indicating total size of all files.
    files.emplace_back(FileInfo{"", offset});

    // Print the file paths and offsets to ensure it works
    // Remove this later
    for (auto const& file : files) {
        std::cout << "File: " << file.n << ", Offset: " << file.o << std::endl;
    }

    return files;
}

auto pack_single_file_archive_metadata(
        clp::streaming_archive::ArchiveMetadata const& multi_file_archive_metadata,
        std::vector<FileInfo> const& file_infos,
        uint64_t const num_segments
) -> std::stringstream {
    ArchiveMetadata archive_metadata{
            .archive_format_version = multi_file_archive_metadata.get_archive_format_version(),
            // TODO: The following fields are required for single-file archive metadata format;
            // however, they are not yet implemented in multi-file archive metadata. Currently,
            // they are set to default values, but should reference the actual values from the
            // multi-file archive metadata once implemented.
            // - variable_encoding_methods_version
            // - variables_schema_version
            // - compression_type
            .variable_encoding_methods_version
            = static_cast<char const*>(ffi::cVariableEncodingMethodsVersion),
            .variables_schema_version = static_cast<char const*>(ffi::cVariablesSchemaVersion),
            .compression_type = std::string(cCompressionTypeZstd),
            .creator_id = multi_file_archive_metadata.get_creator_id(),
            .begin_timestamp = multi_file_archive_metadata.get_begin_timestamp(),
            .end_timestamp = multi_file_archive_metadata.get_end_timestamp(),
            .uncompressed_size = multi_file_archive_metadata.get_uncompressed_size_bytes(),
            .compressed_size = multi_file_archive_metadata.get_compressed_size_bytes(),
    };

    SingleFileArchiveMetadata single_file_archive{
            .archive_files = file_infos,
            .archive_metadata = archive_metadata,
            .num_segments = num_segments,
    };

    std::stringstream buf;
    msgpack::pack(buf, single_file_archive);

    return buf;
}

auto write_archive_header(FileWriter& archive_writer, size_t packed_metadata_size) -> void {
    SingleFileArchiveHeader header{
            .magic{},
            .version = cArchiveVersion,
            .metadata_size = packed_metadata_size,
            .unused{}
    };
    std::memcpy(&header.magic, cUnstructuredSfaMagicNumber.data(), sizeof(header.magic));

    auto char_buffer = std::bit_cast<std::array<char, sizeof(header)>>(header);
    archive_writer.write(char_buffer.data(), char_buffer.size());
}

auto write_archive_metadata(FileWriter& archive_writer, std::stringstream const& packed_metadata)
        -> void {
    archive_writer.write(packed_metadata.str().data(), packed_metadata.str().size());
}

auto write_archive_files(
        FileWriter& archive_writer,
        std::filesystem::path const& multi_file_archive_path,
        std::vector<std::string> const& segment_ids
) -> void {
    for (auto const& file_name : static_archive_file_names) {
        std::filesystem::path static_archive_file_path = multi_file_archive_path / file_name;
        write_archive_file(static_archive_file_path, archive_writer);
    }

    std::filesystem::path segment_dir_path = multi_file_archive_path / cSegmentsDirname;
    for (auto const& segment_id : segment_ids) {
        std::filesystem::path segment_path = segment_dir_path / segment_id;
        write_archive_file(segment_path, archive_writer);
    }
}
}  // namespace clp::streaming_archive::single_file_archive
