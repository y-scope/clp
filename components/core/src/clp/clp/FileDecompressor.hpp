#ifndef CLP_CLP_FILEDECOMPRESSOR_HPP
#define CLP_CLP_FILEDECOMPRESSOR_HPP

#include <cstddef>
#include <string>

#include <boost/filesystem.hpp>

#include "../FileWriter.hpp"
#include "../ir/constants.hpp"
#include "../ir/LogEventSerializer.hpp"
#include "../streaming_archive/MetadataDB.hpp"
#include "../streaming_archive/reader/Archive.hpp"
#include "../streaming_archive/reader/File.hpp"
#include "../streaming_archive/reader/Message.hpp"

namespace clp::clp {
/**
 * Class to hold the data structures that are used to decompress files rather than recreating them
 * within the decompression function or passing them as parameters.
 */
class FileDecompressor {
public:
    // Methods
    bool decompress_file(
            streaming_archive::MetadataDB::FileIterator const& file_metadata_ix,
            std::string const& output_dir,
            streaming_archive::reader::Archive& archive_reader,
            std::unordered_map<std::string, std::string>& temp_path_to_final_path
    );

    template <typename IrOutputHandler>
    auto decompress_to_ir(
            streaming_archive::reader::Archive& archive_reader,
            streaming_archive::MetadataDB::FileIterator const& file_metadata_ix,
            size_t ir_target_size,
            std::string const& temp_output_dir,
            IrOutputHandler ir_output_handler
    ) -> bool;

private:
    // Variables
    FileWriter m_decompressed_file_writer;
    streaming_archive::reader::File m_encoded_file;
    streaming_archive::reader::Message m_encoded_message;
    std::string m_decompressed_message;
};

// Templated methods
template <typename IrOutputHandler>
auto FileDecompressor::decompress_to_ir(
        streaming_archive::reader::Archive& archive_reader,
        streaming_archive::MetadataDB::FileIterator const& file_metadata_ix,
        size_t ir_target_size,
        std::string const& temp_output_dir,
        IrOutputHandler ir_output_handler
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

    ir::LogEventSerializer<ir::four_byte_encoded_variable_t> ir_serializer;
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
                == ir_output_handler(temp_ir_path, file_orig_id, begin_message_ix, end_message_ix))
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
    if (false == ir_output_handler(temp_ir_path, file_orig_id, begin_message_ix, end_message_ix)) {
        return false;
    }

    archive_reader.close_file(m_encoded_file);
    return true;
}
};  // namespace clp::clp

#endif  // CLP_CLP_FILEDECOMPRESSOR_HPP
