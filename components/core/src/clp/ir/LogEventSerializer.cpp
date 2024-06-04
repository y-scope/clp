#include "LogEventSerializer.hpp"

#include <string_utils/string_utils.hpp>

#include "../ErrorCode.hpp"
#include "../ffi/ir_stream/encoding_methods.hpp"
#include "../ffi/ir_stream/protocol_constants.hpp"
#include "spdlog_with_specializations.hpp"

using std::string;
using std::string_view;

namespace clp::ir {
template <typename encoded_variable_t>
LogEventSerializer<encoded_variable_t>::~LogEventSerializer() {
    if (m_is_open) {
        SPDLOG_ERROR("Serializer is not closed before being destroyed - output maybe corrupted");
    }
}

template <typename encoded_variable_t>
auto LogEventSerializer<encoded_variable_t>::open(string const& file_path) -> ErrorCode {
    m_serialized_size = 0;
    m_num_log_events = 0;
    m_ir_buffer.clear();

    m_writer.open(file_path, FileWriter::OpenMode::CREATE_FOR_WRITING);
    m_zstd_compressor.open(m_writer);

    bool res{};
    if constexpr (std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>) {
        m_prev_msg_timestamp = 0;
        res = clp::ffi::ir_stream::four_byte_encoding::serialize_preamble(
                cTimestampPattern,
                cTimestampPatternSyntax,
                cTimezoneID,
                m_prev_msg_timestamp,
                m_ir_buffer
        );
    } else {
        res = clp::ffi::ir_stream::eight_byte_encoding::serialize_preamble(
                cTimestampPattern,
                cTimestampPatternSyntax,
                cTimezoneID,
                m_ir_buffer
        );
    }

    if (false == res) {
        SPDLOG_ERROR("Failed to serialize preamble");
        close_writer();
        return ErrorCode_Failure;
    }

    m_is_open = true;

    // Flush the preamble
    flush();

    return ErrorCode_Success;
}

template <typename encoded_variable_t>
auto LogEventSerializer<encoded_variable_t>::flush() -> void {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }
    m_zstd_compressor.write(
            size_checked_pointer_cast<char const>(m_ir_buffer.data()),
            m_ir_buffer.size()
    );
    m_serialized_size += m_ir_buffer.size();
    m_ir_buffer.clear();
}

template <typename encoded_variable_t>
auto LogEventSerializer<encoded_variable_t>::close() -> void {
    m_ir_buffer.push_back(clp::ffi::ir_stream::cProtocol::Eof);
    flush();
    close_writer();
    m_is_open = false;
}

template <typename encoded_variable_t>
auto LogEventSerializer<encoded_variable_t>::serialize_log_event(
        string_view message,
        epoch_time_ms_t timestamp
) -> ErrorCode {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    string logtype;
    bool res{};
    if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        res = clp::ffi::ir_stream::eight_byte_encoding::serialize_log_event(
                timestamp,
                message,
                logtype,
                m_ir_buffer
        );
    } else {
        auto const timestamp_delta = timestamp - m_prev_msg_timestamp;
        m_prev_msg_timestamp = timestamp;
        res = clp::ffi::ir_stream::four_byte_encoding::serialize_log_event(
                timestamp_delta,
                message,
                logtype,
                m_ir_buffer
        );
    }
    if (false == res) {
        return ErrorCode_Failure;
    }
    m_num_log_events += 1;
    return ErrorCode_Success;
}

template <typename encoded_variable_t>
auto LogEventSerializer<encoded_variable_t>::close_writer() -> void {
    m_zstd_compressor.close();
    m_writer.close();
}

// Explicitly declare template specializations so that we can define the template methods in this
// file
template LogEventSerializer<eight_byte_encoded_variable_t>::~LogEventSerializer();
template LogEventSerializer<four_byte_encoded_variable_t>::~LogEventSerializer();
template auto LogEventSerializer<eight_byte_encoded_variable_t>::open(string const& file_path
) -> ErrorCode;
template auto LogEventSerializer<four_byte_encoded_variable_t>::open(string const& file_path
) -> ErrorCode;
template auto LogEventSerializer<four_byte_encoded_variable_t>::flush() -> void;
template auto LogEventSerializer<eight_byte_encoded_variable_t>::flush() -> void;
template auto LogEventSerializer<four_byte_encoded_variable_t>::close() -> void;
template auto LogEventSerializer<eight_byte_encoded_variable_t>::close() -> void;
template auto LogEventSerializer<eight_byte_encoded_variable_t>::serialize_log_event(
        string_view message,
        epoch_time_ms_t timestamp
) -> ErrorCode;
template auto LogEventSerializer<four_byte_encoded_variable_t>::serialize_log_event(
        string_view message,
        epoch_time_ms_t timestamp
) -> ErrorCode;
template auto LogEventSerializer<four_byte_encoded_variable_t>::close_writer() -> void;
template auto LogEventSerializer<eight_byte_encoded_variable_t>::close_writer() -> void;
}  // namespace clp::ir
