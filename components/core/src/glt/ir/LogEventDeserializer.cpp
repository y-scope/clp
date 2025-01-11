#include "LogEventDeserializer.hpp"

#include <vector>

#include <json/single_include/nlohmann/json.hpp>
#include <outcome/single-header/outcome.hpp>
#include <string_utils/string_utils.hpp>

#include "../ffi/ir_stream/decoding_methods.hpp"
#include "types.hpp"

namespace glt::ir {
template <typename encoded_variable_t>
auto LogEventDeserializer<encoded_variable_t>::create(ReaderInterface& reader)
        -> OUTCOME_V2_NAMESPACE::std_result<LogEventDeserializer<encoded_variable_t>> {
    ffi::ir_stream::encoded_tag_t metadata_type{0};
    std::vector<int8_t> metadata;
    auto ir_error_code = ffi::ir_stream::deserialize_preamble(reader, metadata_type, metadata);
    if (ffi::ir_stream::IRErrorCode_Success != ir_error_code) {
        switch (ir_error_code) {
            case ffi::ir_stream::IRErrorCode_Incomplete_IR:
                return std::errc::result_out_of_range;
            case ffi::ir_stream::IRErrorCode_Corrupted_IR:
            default:
                return std::errc::protocol_error;
        }
    }

    if (ffi::ir_stream::cProtocol::Metadata::EncodingJson != metadata_type) {
        return std::errc::protocol_not_supported;
    }

    // Parse metadata and validate version
    auto metadata_json = nlohmann::json::parse(metadata, nullptr, false);
    if (metadata_json.is_discarded()) {
        return std::errc::protocol_error;
    }
    auto version_iter = metadata_json.find(ffi::ir_stream::cProtocol::Metadata::VersionKey);
    if (metadata_json.end() == version_iter || false == version_iter->is_string()) {
        return std::errc::protocol_error;
    }
    auto metadata_version = version_iter->get_ref<nlohmann::json::string_t&>();
    if (ffi::ir_stream::IRProtocolErrorCode_Supported
        != ffi::ir_stream::validate_protocol_version(metadata_version))
    {
        return std::errc::protocol_not_supported;
    }

    if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        return LogEventDeserializer<encoded_variable_t>{reader};
    }
    if constexpr (std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>) {
        // Get reference timestamp
        auto ref_timestamp_iter
                = metadata_json.find(ffi::ir_stream::cProtocol::Metadata::ReferenceTimestampKey);
        if (metadata_json.end() == ref_timestamp_iter || false == ref_timestamp_iter->is_string()) {
            return std::errc::protocol_error;
        }
        auto ref_timestamp_str = ref_timestamp_iter->get_ref<nlohmann::json::string_t&>();
        epoch_time_ms_t ref_timestamp{};
        if (false == clp::string_utils::convert_string_to_int(ref_timestamp_str, ref_timestamp)) {
            return std::errc::protocol_error;
        }

        return LogEventDeserializer<encoded_variable_t>{reader, ref_timestamp};
    }
}

template <typename encoded_variable_t>
auto LogEventDeserializer<encoded_variable_t>::deserialize_log_event()
        -> OUTCOME_V2_NAMESPACE::std_result<LogEvent<encoded_variable_t>> {
    epoch_time_ms_t timestamp_or_timestamp_delta{};
    std::string logtype;
    std::vector<std::string> dict_vars;
    std::vector<encoded_variable_t> encoded_vars;

    auto ir_error_code = ffi::ir_stream::deserialize_log_event(
            m_reader,
            logtype,
            encoded_vars,
            dict_vars,
            timestamp_or_timestamp_delta
    );
    if (ffi::ir_stream::IRErrorCode_Success != ir_error_code) {
        switch (ir_error_code) {
            case ffi::ir_stream::IRErrorCode_Eof:
                return std::errc::no_message_available;
            case ffi::ir_stream::IRErrorCode_Incomplete_IR:
                return std::errc::result_out_of_range;
            case ffi::ir_stream::IRErrorCode_Corrupted_IR:
            default:
                return std::errc::protocol_error;
        }
    }

    epoch_time_ms_t timestamp{};
    if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
        timestamp = timestamp_or_timestamp_delta;
    } else {  // std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>
        m_prev_msg_timestamp += timestamp_or_timestamp_delta;
        timestamp = m_prev_msg_timestamp;
    }

    return LogEvent<encoded_variable_t>{timestamp, logtype, dict_vars, encoded_vars};
}

// Explicitly declare template specializations so that we can define the template methods in this
// file
template auto LogEventDeserializer<eight_byte_encoded_variable_t>::create(ReaderInterface& reader)
        -> OUTCOME_V2_NAMESPACE::std_result<LogEventDeserializer<eight_byte_encoded_variable_t>>;
template auto LogEventDeserializer<four_byte_encoded_variable_t>::create(ReaderInterface& reader)
        -> OUTCOME_V2_NAMESPACE::std_result<LogEventDeserializer<four_byte_encoded_variable_t>>;
template auto LogEventDeserializer<eight_byte_encoded_variable_t>::deserialize_log_event()
        -> OUTCOME_V2_NAMESPACE::std_result<LogEvent<eight_byte_encoded_variable_t>>;
template auto LogEventDeserializer<four_byte_encoded_variable_t>::deserialize_log_event()
        -> OUTCOME_V2_NAMESPACE::std_result<LogEvent<four_byte_encoded_variable_t>>;
}  // namespace glt::ir
