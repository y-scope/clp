#ifndef CLP_CLP_FILEDECOMPRESSOR_INC
#define CLP_CLP_FILEDECOMPRESSOR_INC

#include <string>

#include "FileDecompressor.hpp"

#include <boost/filesystem.hpp>
#include "../ir/constants.hpp"
#include "../ir/LogEventSerializer.hpp"

using clp::ir::four_byte_encoded_variable_t;
using clp::ir::LogEventSerializer;
using std::string;

namespace clp::clp {
    namespace {
/**
 * Renames a temporary IR file and moves it to the output directory.
 *
 * The new name uses the following format:
 * <orig_file_id>_<begin_message_ix>_<end_message_ix>.clp.zst
 * @param temp_ir
 * @param output_directory
 * @param orig_file_id
 * @param begin_message_ix
 * @param end_message_ix
 * @return Whether the IR file is successfully renamed.
 */
bool rename_ir_file(
    boost::filesystem::path const& temp_ir_path,
    boost::filesystem::path const& output_directory,
    string const& file_orig_id,
    size_t begin_message_ix,
    size_t end_message_ix
) {
    auto ir_file_name = file_orig_id + "_";
    ir_file_name += std::to_string(begin_message_ix) + "_";
    ir_file_name += std::to_string(end_message_ix) + "_";
    ir_file_name += ir::cIrFileExtension;

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
}  // namespace
template <typename OutputFunc>
bool FileDecompressor::decompress_to_ir(
        streaming_archive::MetadataDB::FileIterator const& file_metadata_ix,
        OutputFunc output_func,
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

//    // Generate output directory
//    if (auto const error_code = create_directory_structure(output_dir, 0700);
//        ErrorCode_Success != error_code)
//    {
//        SPDLOG_ERROR(
//            "Failed to create directory structure {}, errno={}",
//            output_dir.c_str(),
//            errno
//        );
//        return false;
//    }
//
//    if (temp_output_dir != output_dir) {
//        // Generate temporary output directory
//        if (auto const error_code = create_directory_structure(temp_output_dir, 0700);
//            ErrorCode_Success != error_code)
//        {
//            SPDLOG_ERROR(
//                "Failed to create directory structure {}, errno={}",
//                output_dir.c_str(),
//                errno
//            );
//            return false;
//        }
//    }

    boost::filesystem::path temp_ir_path{temp_output_dir};
    auto temp_ir_file = m_encoded_file.get_id_as_string();
    temp_ir_file += ir::cIrFileExtension;
    temp_ir_path /= temp_ir_file;

    auto const& file_orig_id = m_encoded_file.get_orig_file_id_as_string();
    auto begin_message_ix = m_encoded_file.get_begin_message_ix();

    LogEventSerializer<four_byte_encoded_variable_t> ir_serializer;
    // Open output IR file
    if (false == ir_serializer.open(temp_ir_path.string())) {
        SPDLOG_ERROR("Failed to serialize preamble");
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

        if (ir_serializer.get_serialized_size() >= ir_target_size) {
            ir_serializer.close();

            auto const end_message_ix = begin_message_ix + ir_serializer.get_num_log_events();
            if (false
                == output_func(
                temp_ir_path,
                file_orig_id,
                begin_message_ix,
                end_message_ix
            ))
            {
                return false;
            }
            begin_message_ix = end_message_ix;

            if (false == ir_serializer.open(temp_ir_path.string())) {
                SPDLOG_ERROR("Failed to serialize preamble");
                return false;
            }
        }

        if (false
            == ir_serializer.serialize_log_event(
            m_encoded_message.get_ts_in_milli(),
            m_decompressed_message
        ))
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

    // NOTE: We don't remove temp_output_dir because we don't know if it existed before this method
    // was called.
    if (false
        == output_func(temp_ir_path, file_orig_id, begin_message_ix, end_message_ix))
    {
        return false;
    }

    archive_reader.close_file(m_encoded_file);
    return true;
}
};  // namespace clp::clp

#endif  // CLP_CLP_FILEDECOMPRESSOR_INC