#include "utils.hpp"

#include <string>

#include <boost/uuid/random_generator.hpp>

#include "../../Defs.h"
#include "../../TimestampPattern.hpp"
#include "Archive.hpp"

using std::string;

namespace glt::streaming_archive::writer {
auto split_archive(Archive::UserConfig& archive_user_config, Archive& archive_writer) -> void {
    archive_writer.close();
    archive_user_config.id = boost::uuids::random_generator()();
    ++archive_user_config.creation_num;
    archive_writer.open(archive_user_config);
}

auto split_file(
        string const& path_for_compression,
        group_id_t group_id,
        TimestampPattern const* last_timestamp_pattern,
        Archive& archive_writer
) -> void {
    auto const& encoded_file = archive_writer.get_file();
    auto orig_file_id = encoded_file.get_orig_file_id();
    auto split_ix = encoded_file.get_split_ix();
    archive_writer.set_file_is_split(true);
    close_file_and_append_to_segment(archive_writer);

    archive_writer.create_and_open_file(path_for_compression, group_id, orig_file_id, ++split_ix);
    // Initialize the file's timestamp pattern to the previous split's pattern
    archive_writer.change_ts_pattern(last_timestamp_pattern);
}

auto split_file_and_archive(
        Archive::UserConfig& archive_user_config,
        string const& path_for_compression,
        group_id_t group_id,
        TimestampPattern const* last_timestamp_pattern,
        Archive& archive_writer
) -> void {
    auto const& encoded_file = archive_writer.get_file();
    auto orig_file_id = encoded_file.get_orig_file_id();
    auto split_ix = encoded_file.get_split_ix();
    archive_writer.set_file_is_split(true);
    close_file_and_append_to_segment(archive_writer);

    split_archive(archive_user_config, archive_writer);

    archive_writer.create_and_open_file(path_for_compression, group_id, orig_file_id, ++split_ix);
    // Initialize the file's timestamp pattern to the previous split's pattern
    archive_writer.change_ts_pattern(last_timestamp_pattern);
}

auto close_file_and_append_to_segment(Archive& archive_writer) -> void {
    archive_writer.close_file();
    archive_writer.append_file_to_segment();
}
}  // namespace glt::streaming_archive::writer
