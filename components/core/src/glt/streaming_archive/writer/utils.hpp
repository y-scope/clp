#ifndef GLT_STREAMING_ARCHIVE_WRITER_UTILS_HPP
#define GLT_STREAMING_ARCHIVE_WRITER_UTILS_HPP

#include <string>

#include "../../Defs.h"
#include "../../TimestampPattern.hpp"
#include "Archive.hpp"

namespace glt::streaming_archive::writer {
/**
 * Closes the current archive and starts a new one
 * @param archive_user_config
 * @param archive_writer
 */
auto split_archive(Archive::UserConfig& archive_user_config, Archive& archive_writer) -> void;

/**
 * Closes the current encoded file in the archive and starts a new one
 * @param path_for_compression
 * @param group_id
 * @param last_timestamp_pattern
 * @param archive_writer
 */
auto split_file(
        std::string const& path_for_compression,
        group_id_t group_id,
        TimestampPattern const* last_timestamp_pattern,
        Archive& archive_writer
) -> void;

/**
 * Closes the archive and its current encoded file, then starts a new archive and encoded file
 * @param archive_user_config
 * @param path_for_compression
 * @param group_id
 * @param last_timestamp_pattern
 * @param archive_writer
 */
auto split_file_and_archive(
        Archive::UserConfig& archive_user_config,
        std::string const& path_for_compression,
        group_id_t group_id,
        TimestampPattern const* last_timestamp_pattern,
        Archive& archive_writer
) -> void;

/**
 * Closes the encoded file in the given archive and appends it to the segment
 * @param archive
 */
auto close_file_and_append_to_segment(Archive& archive) -> void;
}  // namespace glt::streaming_archive::writer

#endif  // GLT_STREAMING_ARCHIVE_WRITER_UTILS_HPP
