#include "FileDecompressor.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "../ir/Constant.hpp"
#include "../ir/LogEventSerializer.hpp"
#include "../ir/utils.hpp"

using clp::ir::four_byte_encoded_variable_t;
using clp::ir::LogEventSerializer;
using std::string;

namespace clp::clp {
namespace {
/**
 * Rename a temporary IR and move it to the output directory
 * The new name follows the following format
 * <FILE_ORIG_ID>_<begin_message_ix>_<end_message_ix>.clp.zst
 * @param temp_ir
 * @param output_directory
 * @param file_orig_id
 * @param begin_message_ix
 * @param end_message_ix
 * @return true if the IR is renamed and moved successfully. Otherwise false
 */
bool rename_ir_file(
        boost::filesystem::path const& temp_ir_path,
        boost::filesystem::path const& output_directory,
        std::string const& file_orig_id,
        size_t begin_message_ix,
        size_t end_message_ix
) {
    std::string ir_file_name = file_orig_id + "_" + std::to_string(begin_message_ix) + "_"
                               + std::to_string(end_message_ix) + "." + ir::cIrExtension;

    auto const renamed_ir_path = output_directory / ir_file_name;
    try {
        boost::filesystem::rename(temp_ir_path, output_directory / ir_file_name);
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

/**
 * Gets an approximated upper bound size of a given log message if it is encoded in IR format.
 * The approximation is based on the following assumptions:
 * 1. Dictionary variable lengths are all encoded using int32_t
 * 2. Timestamp or timestamp delta is encoded using int64_t
 * 3. Logtype length is encoded using int32_t
 * 4. The size of encoded variable roughly equals to their plain text size.
 * @param log_message
 * @param num_encoded_vars
 * @return the approximated ir size in bytes
 */
auto get_approximated_ir_size(std::string_view log_message, size_t num_encoded_vars) -> size_t {
    constexpr size_t cLogtypeLengthSize = sizeof(int32_t);
    constexpr size_t cTagSize = sizeof(char);
    constexpr size_t cVarLengthSize = sizeof(int32_t);
    constexpr size_t cTimestampSize = sizeof(int64_t);
    constexpr size_t cPlaceHolderSize = sizeof(enum_to_underlying_type(ir::VariablePlaceholder()));

    // sizeof(log type) + sizeof (dict vars) + sizeof(encoded_vars) ~= The size of log message
    auto ir_size = log_message.size();

    // Add the size of placeholders in the original log type
    ir_size += num_encoded_vars * cPlaceHolderSize;

    // Add the tags and encoding length bytes of log type
    ir_size += cTagSize + cLogtypeLengthSize;

    // Add the tags and encoding length bytes for dictionary variables
    // Note here we overestimate the size by assuming that encoded float and int also have length
    // encoding bytes
    ir_size += (cTagSize + cVarLengthSize) * num_encoded_vars;

    // Add the tags and encoding length bytes of log type
    ir_size += cTagSize + cTimestampSize;

    return ir_size;
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

bool FileDecompressor::decompress_to_ir(
        streaming_archive::MetadataDB::FileIterator const& file_metadata_ix,
        string const& output_dir,
        string const& temp_output_dir,
        streaming_archive::reader::Archive& archive_reader,
        size_t ir_target_size
) {
    // Open encoded file
    if (auto const error_code = archive_reader.open_file(m_encoded_file, file_metadata_ix);
        ErrorCode_Success != error_code)
    {
        if (ErrorCode_errno == error_code) {
            SPDLOG_ERROR("Failed to open encoded file, errno={}", errno);
        } else {
            SPDLOG_ERROR("Failed to open encoded file, error_code={}", error_code);
        }
        return false;
    }

    // Generate output directory
    if (auto const error_code = create_directory_structure(output_dir, 0700);
        ErrorCode_Success != error_code)
    {
        SPDLOG_ERROR(
                "Failed to create directory structure {}, errno={}",
                output_dir.c_str(),
                errno
        );
        return false;
    }

    if (temp_output_dir != output_dir) {
        // Generate temporary output directory
        if (auto const error_code = create_directory_structure(temp_output_dir, 0700);
            ErrorCode_Success != error_code)
        {
            SPDLOG_ERROR(
                    "Failed to create directory structure {}, errno={}",
                    output_dir.c_str(),
                    errno
            );
            return false;
        }
    }

    boost::filesystem::path temp_ir_path{temp_output_dir};
    temp_ir_path /= m_encoded_file.get_id_as_string() + "." + ir::cIrExtension;

    auto const& file_orig_id = m_encoded_file.get_orig_file_id_as_string();
    auto begin_message_ix = m_encoded_file.get_begin_message_ix();

    LogEventSerializer<four_byte_encoded_variable_t> ir_serializer;
    // Open output IR file
    if (auto const error_code = ir_serializer.open(temp_ir_path.string());
        ErrorCode_Success != error_code)
    {
        return false;
    }

    while (archive_reader.get_next_message(m_encoded_file, m_encoded_message)) {
        if (false
            == archive_reader
                       .decompress_message_without_ts(m_encoded_message, m_decompressed_message))
        {
            SPDLOG_ERROR("Failed to decompress message");
            return false;
        }
        auto const message_size_as_ir = get_approximated_ir_size(
                m_decompressed_message,
                m_encoded_message.get_vars().size()
        );
        if (message_size_as_ir + ir_serializer.get_serialized_size() > ir_target_size) {
            ir_serializer.close();

            auto const end_message_ix = begin_message_ix + ir_serializer.get_num_log_events();
            if (false
                == rename_ir_file(
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

            if (auto const error_code = ir_serializer.open(temp_ir_path.string());
                ErrorCode_Success != error_code)
            {
                return false;
            }
        }

        if (auto const error_code = ir_serializer.serialize_log_event(
                    m_decompressed_message,
                    m_encoded_message.get_ts_in_milli()
            );
            ErrorCode_Success != error_code)
        {
            SPDLOG_ERROR(
                    "Failed to serialize log event: {} with ts {}",
                    m_decompressed_message.c_str(),
                    m_encoded_message.get_ts_in_milli()
            );
            return false;
        }
    }
    auto const end_message_ix = begin_message_ix + ir_serializer.get_num_log_events();
    ir_serializer.close();

    // Note we don't remove the temp_output_dir because we don't know if it exists before execution
    if (false
        == rename_ir_file(temp_ir_path, output_dir, file_orig_id, begin_message_ix, end_message_ix))
    {
        return false;
    }

    // Close files
    archive_reader.close_file(m_encoded_file);
    return true;
}
}  // namespace clp::clp
