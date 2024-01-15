#include "FileDecompressor.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "../spdlog_with_specializations.hpp"

using std::string;

namespace glt::glt {
bool FileDecompressor::decompress_file(
        streaming_archive::MetadataDB::FileIterator const& file_metadata_ix,
        string const& output_dir,
        streaming_archive::reader::Archive& archive_reader,
        std::unordered_map<string, string>& temp_path_to_final_path
) {
    // Open compressed file
    auto error_code = archive_reader.open_file(m_encoded_file, file_metadata_ix);
    if (ErrorCode_Success != error_code) {
        if (ErrorCode_errno == error_code) {
            SPDLOG_ERROR("Failed to open encoded file, errno={}", errno);
        } else {
            SPDLOG_ERROR("Failed to open encoded file, error_code={}", error_code);
        }
        return false;
    }

    boost::filesystem::path final_output_path = output_dir;
    final_output_path /= m_encoded_file.get_orig_path();

    boost::filesystem::path temp_output_path = output_dir;
    FileWriter::OpenMode open_mode;
    boost::system::error_code boost_error_code;
    if (m_encoded_file.is_split() || boost::filesystem::exists(final_output_path, boost_error_code))
    {
        temp_output_path /= m_encoded_file.get_orig_file_id_as_string();
        open_mode = FileWriter::OpenMode::CREATE_IF_NONEXISTENT_FOR_APPENDING;
        auto temp_output_path_string = temp_output_path.string();
        if (0 == temp_path_to_final_path.count(temp_output_path_string)) {
            temp_path_to_final_path[temp_output_path_string] = final_output_path.string();
        }
    } else {
        temp_output_path = final_output_path;
        open_mode = FileWriter::OpenMode::CREATE_FOR_WRITING;
    }

    // Generate output directory
    error_code = create_directory_structure(final_output_path.parent_path().string(), 0700);
    if (ErrorCode_Success != error_code) {
        SPDLOG_ERROR(
                "Failed to create directory structure {}, errno={}",
                final_output_path.parent_path().c_str(),
                errno
        );
        return false;
    }

    // Open output file
    m_decompressed_file_writer.open(temp_output_path.string(), open_mode);

    // Decompress
    archive_reader.reset_file_indices(m_encoded_file);
    while (archive_reader.get_next_message(m_encoded_file, m_encoded_message)) {
        if (!archive_reader
                     .decompress_message(m_encoded_file, m_encoded_message, m_decompressed_message))
        {
            // Can't decompress any more of file
            break;
        }
        m_decompressed_file_writer.write_string(m_decompressed_message);
    }

    // Close files
    m_decompressed_file_writer.close();
    archive_reader.close_file(m_encoded_file);

    return true;
}
}  // namespace glt::glt
