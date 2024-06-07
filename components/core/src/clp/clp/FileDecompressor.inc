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
template <typename IrOutputHandler>
auto FileDecompressor::decompress_to_ir(
        streaming_archive::MetadataDB::FileIterator const& file_metadata_ix,
        streaming_archive::reader::Archive& archive_reader,
        IrOutputHandler ir_output_handler,
        string const& temp_output_dir,
        size_t ir_target_size
) -> bool {
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

    // Generate temporary output directory
    if (auto const error_code = create_directory_structure(temp_output_dir, 0700);
        ErrorCode_Success != error_code)
    {
        SPDLOG_ERROR(
            "Failed to create directory structure {}, errno={}",
            temp_output_dir.c_str(),
            errno
        );
        return false;
    }

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
                == ir_output_handler(
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
        == ir_output_handler(temp_ir_path, file_orig_id, begin_message_ix, end_message_ix))
    {
        return false;
    }

    archive_reader.close_file(m_encoded_file);
    return true;
}
};  // namespace clp::clp

#endif  // CLP_CLP_FILEDECOMPRESSOR_INC