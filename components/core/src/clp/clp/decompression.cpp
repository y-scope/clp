#include "decompression.hpp"

#include <filesystem>
#include <iostream>

#include "../ErrorCode.hpp"
#include "../FileWriter.hpp"
#include "../GlobalMySQLMetadataDB.hpp"
#include "../GlobalSQLiteMetadataDB.hpp"
#include "../ir/constants.hpp"
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
bool decompress(
        CommandLineArguments& command_line_args,
        unordered_set<string> const& files_to_decompress
) {
    ErrorCode error_code;

    // Create output directory in case it doesn't exist
    auto output_dir = std::filesystem::path(command_line_args.get_output_dir());
    error_code = create_directory(output_dir.parent_path().string(), 0700, true);
    if (ErrorCode_Success != error_code) {
        SPDLOG_ERROR("Failed to create {} - {}", output_dir.parent_path().c_str(), strerror(errno));
        return false;
    }

    unordered_set<string> decompressed_files;

    try {
        auto archives_dir = std::filesystem::path(command_line_args.get_archives_dir());
        auto const& global_metadata_db_config = command_line_args.get_metadata_db_config();
        auto global_metadata_db = get_global_metadata_db(global_metadata_db_config, archives_dir);

        streaming_archive::reader::Archive archive_reader;

        std::filesystem::path empty_directory_path;

        FileDecompressor file_decompressor;

        string archive_id;
        string orig_path;
        std::unordered_map<string, string> temp_path_to_final_path;
        global_metadata_db->open();
        if (files_to_decompress.empty()) {
            for (auto archive_ix = std::unique_ptr<GlobalMetadataDB::ArchiveIterator>(
                         global_metadata_db->get_archive_iterator()
                 );
                 archive_ix->contains_element();
                 archive_ix->get_next())
            {
                archive_ix->get_id(archive_id);
                auto archive_path = archives_dir / archive_id;

                if (false == std::filesystem::exists(archive_path)) {
                    SPDLOG_WARN(
                            "Archive {} does not exist in '{}'.",
                            archive_id,
                            command_line_args.get_archives_dir()
                    );
                    continue;
                }

                archive_reader.open(archive_path.string());
                archive_reader.refresh_dictionaries();

                archive_reader.decompress_empty_directories(command_line_args.get_output_dir());

                // Decompress files
                auto file_metadata_ix_ptr = archive_reader.get_file_iterator();
                for (auto& file_metadata_ix = *file_metadata_ix_ptr; file_metadata_ix.has_next();
                     file_metadata_ix.next())
                {
                    // Decompress file
                    if (false
                        == file_decompressor.decompress_file(
                                file_metadata_ix,
                                command_line_args.get_output_dir(),
                                archive_reader,
                                temp_path_to_final_path
                        ))
                    {
                        return false;
                    }
                    file_metadata_ix.get_path(orig_path);
                    decompressed_files.insert(orig_path);
                }
                file_metadata_ix_ptr.reset(nullptr);

                archive_reader.close();
            }
        } else if (files_to_decompress.size() == 1) {
            auto const& file_path = *files_to_decompress.begin();
            for (auto archive_ix = std::unique_ptr<GlobalMetadataDB::ArchiveIterator>(
                         global_metadata_db->get_archive_iterator_for_file_path(file_path)
                 );
                 archive_ix->contains_element();
                 archive_ix->get_next())
            {
                archive_ix->get_id(archive_id);
                auto archive_path = archives_dir / archive_id;
                archive_reader.open(archive_path.string());
                archive_reader.refresh_dictionaries();

                // Decompress all splits with the given path
                auto file_metadata_ix_ptr = archive_reader.get_file_iterator_by_path(file_path);
                for (auto& file_metadata_ix = *file_metadata_ix_ptr; file_metadata_ix.has_next();
                     file_metadata_ix.next())
                {
                    // Decompress file
                    if (false
                        == file_decompressor.decompress_file(
                                file_metadata_ix,
                                command_line_args.get_output_dir(),
                                archive_reader,
                                temp_path_to_final_path
                        ))
                    {
                        return false;
                    }
                    decompressed_files.insert(file_path);
                }
                file_metadata_ix_ptr.reset(nullptr);

                archive_reader.close();
            }
        } else {  // files_to_decompress.size() > 1
            for (auto archive_ix = std::unique_ptr<GlobalMetadataDB::ArchiveIterator>(
                         global_metadata_db->get_archive_iterator()
                 );
                 archive_ix->contains_element();
                 archive_ix->get_next())
            {
                archive_ix->get_id(archive_id);
                auto archive_path = archives_dir / archive_id;
                archive_reader.open(archive_path.string());
                archive_reader.refresh_dictionaries();

                // Decompress files
                auto file_metadata_ix_ptr = archive_reader.get_file_iterator();
                for (auto& file_metadata_ix = *file_metadata_ix_ptr; file_metadata_ix.has_next();
                     file_metadata_ix.next())
                {
                    file_metadata_ix.get_path(orig_path);
                    if (files_to_decompress.count(orig_path) == 0) {
                        // Skip files that aren't in the list of files to decompress
                        continue;
                    }

                    // Decompress file
                    if (false
                        == file_decompressor.decompress_file(
                                file_metadata_ix,
                                command_line_args.get_output_dir(),
                                archive_reader,
                                temp_path_to_final_path
                        ))
                    {
                        return false;
                    }
                    decompressed_files.insert(orig_path);
                }
                file_metadata_ix_ptr.reset(nullptr);

                archive_reader.close();
            }
        }
        global_metadata_db->close();

        string final_path;
        std::error_code std_error_code;
        for (auto const& temp_path_and_final_path : temp_path_to_final_path) {
            final_path = temp_path_and_final_path.second;
            for (size_t i = 1; i < SIZE_MAX; ++i) {
                if (std::filesystem::exists(final_path, std_error_code)) {
                    final_path = temp_path_and_final_path.second;
                    final_path += '.';
                    final_path += std::to_string(i);
                } else {
                    break;
                }
            }
            auto return_value = rename(temp_path_and_final_path.first.c_str(), final_path.c_str());
            if (0 != return_value) {
                SPDLOG_ERROR("Decompression failed - errno={}", errno);
                return false;
            }
        }
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

    if (files_to_decompress.empty() == false) {
        // Check if any requested files were not found in the archive
        for (auto const& file : files_to_decompress) {
            if (decompressed_files.count(file) == 0) {
                SPDLOG_ERROR("'{}' not found in any archive", file.c_str());
            }
        }
    }

    return true;
}

bool decompress_to_ir(CommandLineArguments& command_line_args) {
    ErrorCode error_code{};

    // Create output directory in case it doesn't exist
    std::filesystem::path output_dir{command_line_args.get_output_dir()};
    error_code = create_directory(output_dir.parent_path().string(), 0700, true);
    if (ErrorCode_Success != error_code) {
        SPDLOG_ERROR("Failed to create {} - {}", output_dir.parent_path().c_str(), strerror(errno));
        return false;
    }

    try {
        std::filesystem::path archives_dir{command_line_args.get_archives_dir()};
        auto const& global_metadata_db_config = command_line_args.get_metadata_db_config();
        auto global_metadata_db = get_global_metadata_db(global_metadata_db_config, archives_dir);

        global_metadata_db->open();
        string archive_id;
        string file_split_id;
        if (false
            == global_metadata_db->get_file_split(
                    command_line_args.get_orig_file_id(),
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
        global_metadata_db->close();

        streaming_archive::reader::Archive archive_reader;
        auto archive_path = archives_dir / archive_id;
        archive_reader.open(archive_path.string());
        archive_reader.refresh_dictionaries();

        auto file_metadata_ix_ptr = archive_reader.get_file_iterator_by_split_id(file_split_id);
        if (false == file_metadata_ix_ptr->has_next()) {
            SPDLOG_ERROR("File split doesn't exist {} in archive {}", file_split_id, archive_id);
            return false;
        }

        auto ir_output_handler = [&](std::filesystem::path const& src_ir_path,
                                     string const& orig_file_id,
                                     size_t begin_message_ix,
                                     size_t end_message_ix,
                                     [[maybe_unused]] bool is_last_chunk) {
            auto dest_ir_file_name = orig_file_id;
            dest_ir_file_name += "_" + std::to_string(begin_message_ix);
            dest_ir_file_name += "_" + std::to_string(end_message_ix);
            dest_ir_file_name += ir::cIrFileExtension;

            auto const dest_ir_path = output_dir / dest_ir_file_name;
            try {
                std::filesystem::rename(src_ir_path, dest_ir_path);
            } catch (std::filesystem::filesystem_error const& e) {
                SPDLOG_ERROR(
                        "Failed to rename from {} to {}. Error: {}",
                        src_ir_path.c_str(),
                        dest_ir_path.c_str(),
                        e.what()
                );
                return false;
            }
            return true;
        };

        // Decompress file split
        FileDecompressor file_decompressor;
        if (false
            == file_decompressor.decompress_to_ir(
                    archive_reader,
                    *file_metadata_ix_ptr,
                    command_line_args.get_ir_target_size(),
                    command_line_args.get_output_dir(),
                    ir_output_handler
            ))
        {
            return false;
        }

        file_metadata_ix_ptr.reset(nullptr);

        archive_reader.close();
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
