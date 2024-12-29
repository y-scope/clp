#ifndef CLP_STREAMING_ARCHIVE_SINGLE_FILE_ARCHIVE_WRITER_HPP
#define CLP_STREAMING_ARCHIVE_SINGLE_FILE_ARCHIVE_WRITER_HPP

#include <filesystem>
#include <string>
#include <vector>

#include <msgpack.hpp>

#include "../../Defs.h"
#include "../../ErrorCode.hpp"
#include "../../TraceableException.hpp"
#include "../ArchiveMetadata.hpp"

namespace clp::streaming_archive::single_file_archive {

class OperationFailed : public TraceableException {
public:
    // Constructors
    OperationFailed(
            ErrorCode error_code,
            char const* const filename,
            int line_number,
            std::string message = "streaming_archive::single_file_archive operation failed"
    )
            : TraceableException{error_code, filename, line_number},
              m_message{std::move(message)} {}

    // Methods
    [[nodiscard]] auto what() const noexcept -> char const* override { return m_message.c_str(); }

private:
    std::string m_message;
};

/**
 * @param last_segment_id ID of last written segment in archive.
 * @return Vector of segment IDs.
 */
auto get_segment_ids(segment_id_t last_segment_id) -> std::vector<std::string>;

/**
 * Generates packed single-file archive metadata.
 *
 * @param multi_file_archive_metadata
 * @param multi_file_archive_path
 * @param segment_ids
 * @return Packed metadata.
 */
auto create_single_file_archive_metadata(
        ArchiveMetadata const& multi_file_archive_metadata,
        std::filesystem::path const& multi_file_archive_path,
        std::vector<std::string> const& segment_ids
) -> std::stringstream;

/**
 * Writes header, metadata and archive files in single-file format then
 * removes existing multi-file archive.
 *
 * @param multi_file_archive_path
 * @param packed_metadata
 * @param segment_ids
 * @throws OperationFailed if single-file archive path already exists.
 */
void write_single_file_archive(
        std::filesystem::path const& multi_file_archive_path,
        std::stringstream const& packed_metadata,
        std::vector<std::string> const& segment_ids
);

}  // namespace clp::streaming_archive::single_file_archive

#endif  // CLP_STREAMING_ARCHIVE_SINGLE_FILE_ARCHIVE_WRITER_HPP