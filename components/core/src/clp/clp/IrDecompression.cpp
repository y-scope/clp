#include "IrDecompression.hpp"

#include <iostream>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "../ErrorCode.hpp"
#include "../FileWriter.hpp"
#include "../GlobalMySQLMetadataDB.hpp"
#include "../GlobalSQLiteMetadataDB.hpp"
#include "../spdlog_with_specializations.hpp"
#include "../streaming_archive/reader/Archive.hpp"
#include "../TraceableException.hpp"
#include "../Utils.hpp"
#include "FileDecompressor.hpp"
#include "utils.hpp"

using std::cerr;
using std::make_unique;
using std::string;
using std::unique_ptr;
using std::unordered_set;

namespace clp::clp {
bool decompress_ir(CommandLineArguments& command_line_args, string const& file_orig_id) {
    ErrorCode error_code;

    // Create output directory in case it doesn't exist
    auto output_dir = boost::filesystem::path(command_line_args.get_output_dir());
    error_code = create_directory(output_dir.parent_path().string(), 0700, true);
    if (ErrorCode_Success != error_code) {
        SPDLOG_ERROR("Failed to create {} - {}", output_dir.parent_path().c_str(), strerror(errno));
        return false;
    }

    try {
        auto archives_dir = boost::filesystem::path(command_line_args.get_archives_dir());
        auto const& global_metadata_db_config = command_line_args.get_metadata_db_config();
        auto global_metadata_db = get_global_metadata_db(global_metadata_db_config, archives_dir);

        streaming_archive::reader::Archive archive_reader;

        FileDecompressor file_decompressor;

        string archive_id;
        string file_split_id;
        global_metadata_db->open();

        if (false
            == global_metadata_db->get_file_split(
                    file_orig_id,
                    command_line_args.get_ir_msg_ix(),
                    archive_id,
                    file_split_id
            ))
        {
            SPDLOG_ERROR(
                    "Failed to find file split containing msg_ix {}",
                    command_line_args.get_ir_msg_ix()
            );
            return false;
        }

        auto archive_path = archives_dir / archive_id;
        archive_reader.open(archive_path.string());
        archive_reader.refresh_dictionaries();

        // Decompress the split
        auto file_metadata_ix_ptr = archive_reader.get_file_iterator_by_split_id(file_split_id);
        if (false == file_metadata_ix_ptr->has_next()) {
            SPDLOG_ERROR(
                    "File split doesn't exist {} in the archive {}",
                    file_split_id,
                    archive_id
            );
            return false;
        }
        // Decompress file
        if (false
            == file_decompressor.decompress_to_ir(
                    *file_metadata_ix_ptr,
                    command_line_args.get_output_dir(),
                    command_line_args.get_ir_temp_output_dir(),
                    archive_reader,
                    command_line_args.get_ir_target_size()
            ))
        {
            return false;
        }

        file_metadata_ix_ptr.reset(nullptr);

        archive_reader.close();

        global_metadata_db->close();

    } catch (TraceableException& e) {
        error_code = e.get_error_code();
        if (ErrorCode_errno == error_code) {
            SPDLOG_ERROR(
                    "Decompression failed: {}:{} {}, errno={}",
                    e.get_filename(),
                    e.get_line_number(),
                    e.what(),
                    errno
            );
            return false;
        } else {
            SPDLOG_ERROR(
                    "Decompression failed: {}:{} {}, error_code={}",
                    e.get_filename(),
                    e.get_line_number(),
                    e.what(),
                    error_code
            );
            return false;
        }
    }

    return true;
}
}  // namespace clp::clp
