#include "LogEventSerializer.hpp"

#include <vector>

#include <json/single_include/nlohmann/json.hpp>
#include <string_utils/string_utils.hpp>

#include "../ffi/ir_stream/encoding_methods.hpp"
#include "../ffi/ir_stream/protocol_constants.hpp"
#include "types.hpp"

using std::string;

namespace clp::ir {
template <typename encoded_variable_t>
auto LogEventSerializer<encoded_variable_t>::create(
        WriterInterface& writer,
        epoch_time_ms_t reference_timestamp
)
        -> BOOST_OUTCOME_V2_NAMESPACE::std_result<
                std::unique_ptr<LogEventSerializer<encoded_variable_t>>> {
    static_assert(std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);

    auto serializer_instance = std::unique_ptr<LogEventSerializer<encoded_variable_t>>(
            new LogEventSerializer<encoded_variable_t>{writer, reference_timestamp}
    );
    // For now, use preset values
    if (false
        == serializer_instance->serialize_preamble(
                TIMESTAMP_PATTERN,
                TIMESTAMP_PATTERN_SYNTAX,
                TIME_ZONE_ID,
                reference_timestamp
        ))
    {
        return std::errc::protocol_error;
    }

    return serializer_instance;
}

template <typename encoded_variable_t>
auto LogEventSerializer<encoded_variable_t>::create(WriterInterface& writer)
        -> BOOST_OUTCOME_V2_NAMESPACE::std_result<
                std::unique_ptr<LogEventSerializer<encoded_variable_t>>> {
    static_assert(std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>);

    auto serializer_instance = std::unique_ptr<LogEventSerializer<encoded_variable_t>>(
            new LogEventSerializer<encoded_variable_t>{writer}
    );
    // For now, use preset values
    if (false
        == serializer_instance
                   ->serialize_preamble(TIMESTAMP_PATTERN, TIMESTAMP_PATTERN_SYNTAX, TIME_ZONE_ID))
    {
        return std::errc::protocol_error;
    }
    return serializer_instance;
}

template <typename encoded_variable_t>
auto LogEventSerializer<encoded_variable_t>::serialize_preamble(
        string_view timestamp_pattern,
        string_view timestamp_pattern_syntax,
        string_view time_zone_id
) -> bool {
    static_assert(std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>);
    return clp::ffi::ir_stream::eight_byte_encoding::serialize_preamble(
            timestamp_pattern,
            timestamp_pattern_syntax,
            time_zone_id,
            m_ir_buffer
    );
}

template <typename encoded_variable_t>
auto LogEventSerializer<encoded_variable_t>::serialize_preamble(
        string_view timestamp_pattern,
        string_view timestamp_pattern_syntax,
        string_view time_zone_id,
        epoch_time_ms_t reference_timestamp
) -> bool {
    static_assert(std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>);
    return clp::ffi::ir_stream::four_byte_encoding::serialize_preamble(
            timestamp_pattern,
            timestamp_pattern_syntax,
            time_zone_id,
            reference_timestamp,
            m_ir_buffer
    );
}

template <typename encoded_variable_t>
auto LogEventSerializer<encoded_variable_t>::serialize_log_event(
        string_view message,
        epoch_time_ms_t timestamp
) -> bool {
    string logtype;
    // Increment m_log_event_ix without waiting for the return of serialize_log_event.
    // If serialize_log_event fails, the IR will be corrupted anyway.
    m_log_event_ix++;
    if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        return clp::ffi::ir_stream::eight_byte_encoding::serialize_log_event(
                timestamp,
                message,
                logtype,
                m_ir_buffer
        );
    } else {
        auto timestamp_delta = timestamp - m_prev_msg_timestamp;
        m_prev_msg_timestamp = timestamp;
        return clp::ffi::ir_stream::four_byte_encoding::serialize_log_event(
                timestamp_delta,
                message,
                logtype,
                m_ir_buffer
        );
    }
}

template <typename encoded_variable_t>
auto LogEventSerializer<encoded_variable_t>::close() -> void {
    m_ir_buffer.push_back(clp::ffi::ir_stream::cProtocol::Eof);
    flush();
}

template <typename encoded_variable_t>
auto LogEventSerializer<encoded_variable_t>::flush() -> void {
    m_writer.write(reinterpret_cast<char const*>(m_ir_buffer.data()), m_ir_buffer.size());
    m_serialized_size += m_ir_buffer.size();
    m_ir_buffer.clear();
}

// Explicitly declare template specializations so that we can define the template methods in this
// file
template auto LogEventSerializer<eight_byte_encoded_variable_t>::create(WriterInterface& writer)
        -> BOOST_OUTCOME_V2_NAMESPACE::std_result<
                std::unique_ptr<LogEventSerializer<eight_byte_encoded_variable_t>>>;
template auto LogEventSerializer<four_byte_encoded_variable_t>::create(
        WriterInterface& writer,
        epoch_time_ms_t reference_timestamp
)
        -> BOOST_OUTCOME_V2_NAMESPACE::std_result<
                std::unique_ptr<LogEventSerializer<four_byte_encoded_variable_t>>>;
template auto LogEventSerializer<eight_byte_encoded_variable_t>::serialize_preamble(
        string_view timestamp_pattern,
        string_view timestamp_pattern_syntax,
        string_view time_zone_id
) -> bool;
template auto LogEventSerializer<four_byte_encoded_variable_t>::serialize_preamble(
        string_view timestamp_pattern,
        string_view timestamp_pattern_syntax,
        string_view time_zone_id,
        epoch_time_ms_t reference_timestamp
) -> bool;
template auto LogEventSerializer<eight_byte_encoded_variable_t>::serialize_log_event(
        string_view message,
        epoch_time_ms_t timestamp
) -> bool;
template auto LogEventSerializer<four_byte_encoded_variable_t>::serialize_log_event(
        string_view message,
        epoch_time_ms_t timestamp
) -> bool;
template auto LogEventSerializer<four_byte_encoded_variable_t>::flush() -> void;
template auto LogEventSerializer<eight_byte_encoded_variable_t>::flush() -> void;
template auto LogEventSerializer<four_byte_encoded_variable_t>::close() -> void;
template auto LogEventSerializer<eight_byte_encoded_variable_t>::close() -> void;
}  // namespace clp::ir
