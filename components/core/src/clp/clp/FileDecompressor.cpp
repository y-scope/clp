#include "FileDecompressor.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "../ir/LogEventSerializer.hpp"
#include "../ir/utils.hpp"

using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::four_byte_encoded_variable_t;
using clp::ir::LogEventSerializer;
using std::string;

namespace clp::clp {
namespace {
/**
 * Rename the temporary IR chunk and move it to the output directory
 * The name name follows the following format
 * <FILE_ORIG_ID>_<begin_msg_ix>_<end_msg_ix>.clp.zst
 * @param temp_ir
 * @param output_directory
 * @param file_orig_id
 * @param begin_msg_ix
 * @param end_msg_ix
 * @return true if the IR is renamed and moved, false otherwise
 */
bool rename_and_move_ir(
        boost::filesystem::path const& temp_ir_path,
        boost::filesystem::path const& output_directory,
        std::string const& file_orig_id,
        size_t begin_msg_ix,
        size_t end_msg_ix
) {
    std::string ir_name = file_orig_id + "_" + std::to_string(begin_msg_ix) + "_"
                          + std::to_string(end_msg_ix) + ".clp.zst";

    auto renamed_ir_path = output_directory / ir_name;
    try {
        boost::filesystem::rename(temp_ir_path, output_directory / ir_name);
    } catch (boost::filesystem::filesystem_error const& e) {
        SPDLOG_ERROR(
                "Failed to rename from {} to {}. Error: {}",
                temp_ir_path.c_str(),
                renamed_ir_path.c_str(),
                e.what()
        );
        return false;
    }
    return true;
}
}  // namespace

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

bool FileDecompressor::decompress_ir(
        streaming_archive::MetadataDB::FileIterator const& file_metadata_ix,
        string const& output_dir,
        string const& temp_output_dir,
        streaming_archive::reader::Archive& archive_reader,
        size_t ir_target_size
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

    // Generate output directory
    error_code = create_directory_structure(output_dir, 0700);
    if (ErrorCode_Success != error_code) {
        SPDLOG_ERROR(
                "Failed to create directory structure {}, errno={}",
                output_dir.c_str(),
                errno
        );
        return false;
    }

    if (temp_output_dir != output_dir) {
        // Generate temporary output directory
        error_code = create_directory_structure(temp_output_dir, 0700);
        if (ErrorCode_Success != error_code) {
            SPDLOG_ERROR(
                    "Failed to create directory structure {}, errno={}",
                    output_dir.c_str(),
                    errno
            );
            return false;
        }
    }

    boost::filesystem::path temp_ir_path{temp_output_dir};
    temp_ir_path /= m_encoded_file.get_id_as_string() + ".temp.clp.zst";

    auto const& file_orig_id = m_encoded_file.get_orig_file_id_as_string();
    auto begin_message_ix = m_encoded_file.get_begin_message_ix();

    // Open output file
    streaming_compression::zstd::Compressor ir_compressor;
    m_decompressed_file_writer.open(
            temp_ir_path.string(),
            FileWriter::OpenMode::CREATE_FOR_WRITING
    );
    ir_compressor.open(m_decompressed_file_writer);

    auto result = LogEventSerializer<four_byte_encoded_variable_t>::create(
            ir_compressor,
            m_encoded_file.get_begin_ts()
    );
    if (result.has_error()) {
        SPDLOG_ERROR("Failed to create Serializer: {}", result.error().message());
        return false;
    }
    LogEventSerializer<four_byte_encoded_variable_t>* serializer_inst = result.value().get();

    while (archive_reader.get_next_message(m_encoded_file, m_encoded_message)) {
        if (!archive_reader
                     .decompress_message_without_ts(m_encoded_message, m_decompressed_message))
        {
            SPDLOG_ERROR("Failed to decompress message");
            return false;
        }
        auto const message_size_as_ir = ir::get_approximated_ir_size(
                m_decompressed_message,
                m_encoded_message.get_vars().size()
        );
        if (message_size_as_ir + serializer_inst->get_serialized_size() > ir_target_size) {
            serializer_inst->flush();
            ir_compressor.close();
            m_decompressed_file_writer.close();

            auto end_message_ix = begin_message_ix + serializer_inst->get_log_event_ix();
            if (false
                == rename_and_move_ir(
                        temp_ir_path,
                        output_dir,
                        file_orig_id,
                        begin_message_ix,
                        end_message_ix
                ))
            {
                return false;
            }
            begin_message_ix = end_message_ix;

            m_decompressed_file_writer.open(
                    temp_ir_path.string(),
                    FileWriter::OpenMode::CREATE_FOR_WRITING
            );
            ir_compressor.open(m_decompressed_file_writer);

            result = LogEventSerializer<four_byte_encoded_variable_t>::create(
                    ir_compressor,
                    m_encoded_message.get_ts_in_milli()
            );
            if (result.has_error()) {
                SPDLOG_ERROR("Failed to create Serializer: {}", result.error().message());
                return false;
            }
            serializer_inst = result.value().get();
        }

        if (false
            == serializer_inst->serialize_log_event(
                    m_decompressed_message,
                    m_encoded_message.get_ts_in_milli()
            ))
        {
            SPDLOG_ERROR("Failed to serialize log event: {}", m_decompressed_message.c_str());
            return false;
        }
    }

    serializer_inst->flush();
    ir_compressor.close();
    m_decompressed_file_writer.close();

    if (false
        == rename_and_move_ir(
                temp_ir_path,
                output_dir,
                file_orig_id,
                begin_message_ix,
                begin_message_ix + serializer_inst->get_log_event_ix()
        ))
    {
        return false;
    }

    // Close files
    archive_reader.close_file(m_encoded_file);
    return true;
}
}  // namespace clp::clp
