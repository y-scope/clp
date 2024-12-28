#ifndef CLP_STREAMING_ARCHIVE_SINGLE_FILE_ARCHIVE_UTILS_HPP
#define CLP_STREAMING_ARCHIVE_SINGLE_FILE_ARCHIVE_UTILS_HPP

#include <filesystem>
#include <string>
#include <vector>

#include <msgpack.hpp>

#include "../../ErrorCode.hpp"
#include "../../FileWriter.hpp"
#include "../../TraceableException.hpp"
#include "../ArchiveMetadata.hpp"
#include "Defs.hpp"

namespace clp::streaming_archive::single_file_archive {

class OperationFailed : public TraceableException {
public:
    // Constructors
    OperationFailed(
            ErrorCode error_code,
            char const* const filename,
            int line_number,
            std::string message
            = "streaming_archive::writer::single_file_archive::utils operation failed"
    )
            : TraceableException{error_code, filename, line_number},
              m_message{std::move(message)} {}

    // Methods
    [[nodiscard]] auto what() const noexcept -> char const* override { return m_message.c_str(); }

private:
    std::string m_message;
};

/**
 * @param last_segment_id ID of last written segment.
 * @return Vector of segment IDs.
 */
auto get_segment_ids(segment_id_t last_segment_id) -> std::vector<std::string>;

auto create_single_file_archive_metadata(
        clp::streaming_archive::ArchiveMetadata const& multi_file_archive_metadata,
        std::filesystem::path const& multi_file_archive_path,
        std::vector<std::string> const& segment_ids
) -> std::stringstream;

/**
 * @param multi_file_archive_path Path to the multi-file archive.
 * @param segment_ids Vector of segment IDs.
 * @return Vector containing a `FileInfo` struct for every file in the multi-file archive.
 * @throws OperationFailed if error getting file size.
 */
auto get_file_infos(
        std::filesystem::path const& multi_file_archive_path,
        std::vector<std::string> const& segment_ids
) -> std::vector<FileInfo>;

/**
 * Serializes single-file archive metadata into MsgPack.
 *
 * @param multi_file_archive_metadata Multi-file archive metadata.
 * @param file_infos Vector containing a `FileInfo` struct for every file in the multi-file archive.
 * @param segment_ids Vector of segment IDs.
 * @return Packed metadata.
 */
auto pack_single_file_archive_metadata(
        clp::streaming_archive::ArchiveMetadata const& multi_file_archive_metadata,
        std::vector<FileInfo> const& file_infos,
        std::vector<std::string> const& segment_ids
) -> std::stringstream;

/**
 * Writes single-file archive header.
 *
 * @param archive_writer
 * @param metadata_size
 */
auto write_archive_header(FileWriter& archive_writer, size_t packed_metadata_size) -> void;

/**
 * Writes single-file archive metadata.
 *
 * @param archive_writer
 * @param packed_metadata Packed metadata.
 */
auto write_archive_metadata(FileWriter& archive_writer, std::stringstream const& packed_metadata)
        -> void;

/**
 * Iterates over files in the multi-file archive copying their contents to the single-file archive.
 * Skips metadata since already written in `write_archive_metadata`.
 *
 * @param archive_writer
 * @param multi_file_archive_path
 * @param segment_ids Vector of segment IDs.
 * @throws OperationFailed if reading a file fails.
 */
auto write_archive_files(
        FileWriter& archive_writer,
        std::filesystem::path const& multi_file_archive_path,
        std::vector<std::string> const& segment_ids
) -> void;

}  // namespace clp::streaming_archive::single_file_archive

#endif  // CLP_STREAMING_ARCHIVE_SINGLE_FILE_ARCHIVE_UTILS_HPP
