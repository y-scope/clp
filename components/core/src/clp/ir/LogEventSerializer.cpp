#include "LogEventSerializer.hpp"

#include <string>
#include <string_view>

#include <spdlog/spdlog.h>

#include "../Defs.h"
#include "../ErrorCode.hpp"
#include "../ffi/ir_stream/encoding_methods.hpp"
#include "../ffi/ir_stream/protocol_constants.hpp"
#include "../ir/types.hpp"
#include "../type_utils.hpp"

using std::string;
using std::string_view;

namespace clp::ir {
template <typename encoded_variable_t>
LogEventSerializer<encoded_variable_t>::~LogEventSerializer() {
    if (m_is_open) {
        SPDLOG_ERROR("clp::ir::LogEventSerializer not closed before being destroyed - output maybe "
                     "corrupted.");
    }
}

template <typename encoded_variable_t>
auto LogEventSerializer<encoded_variable_t>::open(string const& file_path) -> bool {
    if (m_is_open) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    m_serialized_size = 0;
    m_num_log_events = 0;
    m_ir_buf.clear();

    m_writer.open(file_path, FileWriter::OpenMode::CREATE_FOR_WRITING);
    m_zstd_compressor.open(m_writer);

    bool res{};
    if constexpr (std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>) {
        m_prev_event_timestamp = 0;
        res = ffi::ir_stream::four_byte_encoding::serialize_preamble(
                cTimestampPattern,
                cTimestampPatternSyntax,
                cTimezoneID,
                m_prev_event_timestamp,
                m_ir_buf
        );
    } else {
        res = clp::ffi::ir_stream::eight_byte_encoding::serialize_preamble(
                cTimestampPattern,
                cTimestampPatternSyntax,
                cTimezoneID,
                m_ir_buf
        );
    }

    if (false == res) {
        close_writer();
        return false;
    }

    m_is_open = true;

    // Flush the preamble
    flush();

    return true;
}

template <typename encoded_variable_t>
auto LogEventSerializer<encoded_variable_t>::flush() -> void {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }
    m_zstd_compressor.write(
            size_checked_pointer_cast<char const>(m_ir_buf.data()),
            m_ir_buf.size()
    );
    m_ir_buf.clear();
}

template <typename encoded_variable_t>
auto LogEventSerializer<encoded_variable_t>::close() -> void {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }
    m_ir_buf.push_back(clp::ffi::ir_stream::cProtocol::Eof);
    flush();
    close_writer();
    m_is_open = false;
}

template <typename encoded_variable_t>
auto LogEventSerializer<encoded_variable_t>::serialize_log_event(
        epoch_time_ms_t timestamp,
        string_view message
) -> bool {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
    }

    string logtype;
    bool res{};
    auto const buf_size_before_serialization = m_ir_buf.size();
    if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        res = clp::ffi::ir_stream::eight_byte_encoding::serialize_log_event(
                timestamp,
                message,
                logtype,
                m_ir_buf
        );
    } else {
        auto const timestamp_delta = timestamp - m_prev_event_timestamp;
        m_prev_event_timestamp = timestamp;
        res = clp::ffi::ir_stream::four_byte_encoding::serialize_log_event(
                timestamp_delta,
                message,
                logtype,
                m_ir_buf
        );
    }
    if (false == res) {
        return false;
    }
    m_serialized_size += m_ir_buf.size() - buf_size_before_serialization;
    ++m_num_log_events;
    return true;
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
template auto LogEventSerializer<eight_byte_encoded_variable_t>::open(string const& file_path)
        -> bool;
template auto LogEventSerializer<four_byte_encoded_variable_t>::open(string const& file_path)
        -> bool;
template auto LogEventSerializer<eight_byte_encoded_variable_t>::flush() -> void;
template auto LogEventSerializer<four_byte_encoded_variable_t>::flush() -> void;
template auto LogEventSerializer<eight_byte_encoded_variable_t>::close() -> void;
template auto LogEventSerializer<four_byte_encoded_variable_t>::close() -> void;
template auto LogEventSerializer<eight_byte_encoded_variable_t>::serialize_log_event(
        epoch_time_ms_t timestamp,
        string_view message
) -> bool;
template auto LogEventSerializer<four_byte_encoded_variable_t>::serialize_log_event(
        epoch_time_ms_t timestamp,
        string_view message
) -> bool;
template auto LogEventSerializer<eight_byte_encoded_variable_t>::close_writer() -> void;
template auto LogEventSerializer<four_byte_encoded_variable_t>::close_writer() -> void;
}  // namespace clp::ir
